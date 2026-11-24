/*
 * Copyright (c) 2013-2021 Apple Inc. All rights reserved.
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

#ifndef _NET_NECP_H_
#define _NET_NECP_H_

#include <net/net_kev.h>
#ifdef PRIVATE

#include <netinet/in.h>
#include <netinet/in_stat.h>
#include <sys/socket.h>
#include <net/if.h>

  
                              
   
#define NECP_CONTROL_NAME "com.apple.net.necp_control"

#define NECP_TLV_LENGTH_UINT32  1

struct necp_packet_header {
	u_int8_t            packet_type;
	u_int8_t                flags;
	u_int32_t           message_id;
};

  
                           
   
#define NECP_PACKET_TYPE_POLICY_ADD                             1
#define NECP_PACKET_TYPE_POLICY_GET                             2
#define NECP_PACKET_TYPE_POLICY_DELETE                  3
#define NECP_PACKET_TYPE_POLICY_APPLY_ALL               4
#define NECP_PACKET_TYPE_POLICY_LIST_ALL                5
#define NECP_PACKET_TYPE_POLICY_DELETE_ALL              6
#define NECP_PACKET_TYPE_SET_SESSION_PRIORITY   7
#define NECP_PACKET_TYPE_LOCK_SESSION_TO_PROC   8
#define NECP_PACKET_TYPE_REGISTER_SERVICE               9
#define NECP_PACKET_TYPE_UNREGISTER_SERVICE             10
#define NECP_PACKET_TYPE_POLICY_DUMP_ALL                11

  
                  
   
#define NECP_SESSION_ACTION_POLICY_ADD                          1                                                
#define NECP_SESSION_ACTION_POLICY_GET                          2                                               
#define NECP_SESSION_ACTION_POLICY_DELETE                       3                                        
#define NECP_SESSION_ACTION_POLICY_APPLY_ALL            4                                 
#define NECP_SESSION_ACTION_POLICY_LIST_ALL                     5                                        
#define NECP_SESSION_ACTION_POLICY_DELETE_ALL           6                                 
#define NECP_SESSION_ACTION_SET_SESSION_PRIORITY        7                                             
#define NECP_SESSION_ACTION_LOCK_SESSION_TO_PROC        8                                 
#define NECP_SESSION_ACTION_REGISTER_SERVICE            9                                  
#define NECP_SESSION_ACTION_UNREGISTER_SERVICE          10                                 
#define NECP_SESSION_ACTION_POLICY_DUMP_ALL                     11                                                                   

  
                        
   
#define NECP_PACKET_FLAGS_RESPONSE                              0x01                                                 

  
                            
   
#define NECP_TLV_NIL                                                    0
#define NECP_TLV_ERROR                                                  1                   
#define NECP_TLV_POLICY_ORDER                                   2                   
#define NECP_TLV_POLICY_CONDITION                               3
#define NECP_TLV_POLICY_RESULT                                  4
#define NECP_TLV_POLICY_ID                                              5                   
#define NECP_TLV_SESSION_PRIORITY                               6                   
#define NECP_TLV_ATTRIBUTE_DOMAIN                               7                
#define NECP_TLV_ATTRIBUTE_ACCOUNT                              8                
#define NECP_TLV_SERVICE_UUID                                   9                
#define NECP_TLV_ROUTE_RULE                                             10

  
                                                           
   
#define NECP_TLV_POLICY_OWNER                                   100               
#define NECP_TLV_POLICY_DUMP                                    101
#define NECP_TLV_POLICY_RESULT_STRING                   102               
#define NECP_TLV_POLICY_SESSION_ORDER                   103                 

  
                  
   
#define NECP_POLICY_CONDITION_FLAGS_NEGATIVE    0x01            

  
             
                                                                                
   
#define NECP_POLICY_CONDITION_DEFAULT                   0                                                  
                                
#define NECP_POLICY_CONDITION_APPLICATION               1                                                   
#define NECP_POLICY_CONDITION_REAL_APPLICATION  2                                                                                              
#define NECP_POLICY_CONDITION_DOMAIN                    3                                   
#define NECP_POLICY_CONDITION_ACCOUNT                   4                
#define NECP_POLICY_CONDITION_ENTITLEMENT               5                
#define NECP_POLICY_CONDITION_PID                               6               
#define NECP_POLICY_CONDITION_UID                               7               
#define NECP_POLICY_CONDITION_ALL_INTERFACES    8             
#define NECP_POLICY_CONDITION_BOUND_INTERFACE   9                
#define NECP_POLICY_CONDITION_TRAFFIC_CLASS             10                                       
                       
#define NECP_POLICY_CONDITION_IP_PROTOCOL               11                  
#define NECP_POLICY_CONDITION_LOCAL_ADDR                12                                   
#define NECP_POLICY_CONDITION_REMOTE_ADDR               13                                   
#define NECP_POLICY_CONDITION_LOCAL_ADDR_RANGE  14                                         
#define NECP_POLICY_CONDITION_REMOTE_ADDR_RANGE 15                                         
#define NECP_POLICY_CONDITION_AGENT_TYPE                16                                                
#define NECP_POLICY_CONDITION_HAS_CLIENT                17            
#define NECP_POLICY_CONDITION_LOCAL_NETWORKS            18                                   
                         
#define NECP_POLICY_CONDITION_FLOW_IP_PROTOCOL          19                  
#define NECP_POLICY_CONDITION_FLOW_LOCAL_ADDR           20                                   
#define NECP_POLICY_CONDITION_FLOW_REMOTE_ADDR          21                                   
#define NECP_POLICY_CONDITION_FLOW_LOCAL_ADDR_RANGE     22                                         
#define NECP_POLICY_CONDITION_FLOW_REMOTE_ADDR_RANGE    23                                         
#define NECP_POLICY_CONDITION_FLOW_IS_LOOPBACK          31            
                                           
#define NECP_POLICY_CONDITION_CLIENT_FLAGS              24                                                            
#define NECP_POLICY_CONDITION_FLOW_LOCAL_ADDR_EMPTY     25            
#define NECP_POLICY_CONDITION_FLOW_REMOTE_ADDR_EMPTY    26            
#define NECP_POLICY_CONDITION_PLATFORM_BINARY           27            
#define NECP_POLICY_CONDITION_SDK_VERSION               28                                                 
#define NECP_POLICY_CONDITION_SIGNING_IDENTIFIER        29               
#define NECP_POLICY_CONDITION_PACKET_FILTER_TAGS        30                  
#define NECP_POLICY_CONDITION_DELEGATE_IS_PLATFORM_BINARY      32            

  
                     
   
#define NECP_POLICY_CONDITION_PACKET_FILTER_TAG_STACK_DROP         0x01
#define NECP_POLICY_CONDITION_PACKET_FILTER_TAG_MAX                NECP_POLICY_CONDITION_PACKET_FILTER_TAG_STACK_DROP

  
          
   
#define NECP_POLICY_RESULT_PASS                                 1             
#define NECP_POLICY_RESULT_SKIP                                 2                                                                             
#define NECP_POLICY_RESULT_DROP                                 3             
#define NECP_POLICY_RESULT_SOCKET_DIVERT                4                                             
#define NECP_POLICY_RESULT_SOCKET_FILTER                5                                        
#define NECP_POLICY_RESULT_IP_TUNNEL                    6                                
#define NECP_POLICY_RESULT_IP_FILTER                    7           
#define NECP_POLICY_RESULT_TRIGGER                              8                        
#define NECP_POLICY_RESULT_TRIGGER_IF_NEEDED    9                        
#define NECP_POLICY_RESULT_TRIGGER_SCOPED               10                       
#define NECP_POLICY_RESULT_NO_TRIGGER_SCOPED    11                       
#define NECP_POLICY_RESULT_SOCKET_SCOPED                12                               
#define NECP_POLICY_RESULT_ROUTE_RULES                  13                                           
#define NECP_POLICY_RESULT_USE_NETAGENT                 14                        
#define NECP_POLICY_RESULT_NETAGENT_SCOPED              15                        
#define NECP_POLICY_RESULT_SCOPED_DIRECT                16                                                  
#define NECP_POLICY_RESULT_ALLOW_UNENTITLED             17            

#define NECP_POLICY_RESULT_MAX                          NECP_POLICY_RESULT_ALLOW_UNENTITLED

  
                    
   
#define NECP_POLICY_PASS_NO_SKIP_IPSEC                  0x01
#define NECP_POLICY_PASS_PF_TAG                         0x02

  
                    
   
#define NECP_POLICY_DROP_FLAG_LOCAL_NETWORK             0x01

  
              
                                                          
   
#define NECP_ROUTE_RULE_NONE                                    0             
#define NECP_ROUTE_RULE_DENY_INTERFACE                  1                                       
#define NECP_ROUTE_RULE_ALLOW_INTERFACE                 2                                       
#define NECP_ROUTE_RULE_QOS_MARKING                             3                                       
#define NECP_ROUTE_RULE_DENY_LQM_ABORT                  4                                       
#define NECP_ROUTE_RULE_USE_NETAGENT                    5                                           

#define NECP_ROUTE_RULE_FLAG_CELLULAR                   0x01
#define NECP_ROUTE_RULE_FLAG_WIFI                       0x02
#define NECP_ROUTE_RULE_FLAG_WIRED                      0x04
#define NECP_ROUTE_RULE_FLAG_EXPENSIVE                  0x08
#define NECP_ROUTE_RULE_FLAG_CONSTRAINED                0x10

  
              
   
#define NECP_ERROR_INTERNAL                                             0
#define NECP_ERROR_UNKNOWN_PACKET_TYPE                  1
#define NECP_ERROR_INVALID_TLV                                  2
#define NECP_ERROR_POLICY_RESULT_INVALID                3
#define NECP_ERROR_POLICY_CONDITIONS_INVALID    4
#define NECP_ERROR_POLICY_ID_NOT_FOUND                  5
#define NECP_ERROR_INVALID_PROCESS                              6
#define NECP_ERROR_ROUTE_RULES_INVALID                  7

            
#define NECP_MASK_USERSPACE_ONLY        0x80000000                                     

struct necp_policy_condition_tc_range {
	u_int32_t start_tc;
	u_int32_t end_tc;
} __attribute__((__packed__));

struct necp_policy_condition_addr {
	u_int8_t                prefix;
	union {
		struct sockaddr                 sa;
		struct sockaddr_in              sin;
		struct sockaddr_in6             sin6;
	} address;
} __attribute__((__packed__));

struct necp_policy_condition_addr_range {
	union {
		struct sockaddr                 sa;
		struct sockaddr_in              sin;
		struct sockaddr_in6             sin6;
	} start_address;
	union {
		struct sockaddr                 sa;
		struct sockaddr_in              sin;
		struct sockaddr_in6             sin6;
	} end_address;
} __attribute__((__packed__));

struct necp_policy_condition_agent_type {
	char agent_domain[32];
	char agent_type[32];
} __attribute__((__packed__));

struct necp_policy_condition_sdk_version {
	uint32_t platform;                      
	uint32_t min_version;                         
	uint32_t version;                         
} __attribute__((__packed__));

#define NECP_SESSION_PRIORITY_UNKNOWN                   0
#define NECP_SESSION_PRIORITY_CONTROL                   1
#define NECP_SESSION_PRIORITY_PRIVILEGED_TUNNEL         2
#define NECP_SESSION_PRIORITY_HIGH                      3
#define NECP_SESSION_PRIORITY_HIGH_1                    4
#define NECP_SESSION_PRIORITY_HIGH_2                    5
#define NECP_SESSION_PRIORITY_HIGH_3                    6
#define NECP_SESSION_PRIORITY_HIGH_4                    7
#define NECP_SESSION_PRIORITY_HIGH_RESTRICTED           8
#define NECP_SESSION_PRIORITY_DEFAULT                   9
#define NECP_SESSION_PRIORITY_LOW                       10
#define NECP_SESSION_NUM_PRIORITIES                             NECP_SESSION_PRIORITY_LOW

typedef u_int32_t necp_policy_id;
typedef u_int32_t necp_policy_order;
typedef u_int32_t necp_session_priority;

typedef u_int32_t necp_kernel_policy_result;
typedef u_int32_t necp_kernel_policy_filter;

typedef union {
	u_int                                           tunnel_interface_index;
	u_int                                           scoped_interface_index;
	u_int32_t                                       flow_divert_control_unit;
	u_int32_t                                       filter_control_unit;
	u_int32_t                                       pass_flags;
	u_int32_t                                       drop_flags;
} necp_kernel_policy_routing_result_parameter;

#define NECP_SERVICE_FLAGS_REGISTERED                   0x01
#define NECP_MAX_NETAGENTS                                              8

#define NECP_AGENT_USE_FLAG_SCOPE                               0x01

#define NECP_TFO_COOKIE_LEN_MAX      16
struct necp_aggregate_result {
	necp_kernel_policy_result                       routing_result;
	necp_kernel_policy_routing_result_parameter     routing_result_parameter;
	necp_kernel_policy_filter                       filter_control_unit;
	u_int32_t                                       flow_divert_aggregate_unit;
	necp_kernel_policy_result                       service_action;
	uuid_t                                                          service_uuid;
	u_int32_t                                                       service_flags;
	u_int32_t                                                       service_data;
	u_int                                                           routed_interface_index;
	u_int32_t                                                       policy_id;
	uuid_t                                                          netagents[NECP_MAX_NETAGENTS];
	u_int32_t                                                       netagent_use_flags[NECP_MAX_NETAGENTS];
	struct ipv6_prefix                                              nat64_prefixes[NAT64_MAX_NUM_PREFIXES];
	u_int8_t                                                        mss_recommended;
};

  
                                                                              
                                                                                           
                                     
   
struct necp_stat_counts {
	              
	u_int64_t       necp_stat_rxpackets             __attribute__((aligned(8)));
	u_int64_t       necp_stat_rxbytes               __attribute__((aligned(8)));
	u_int64_t       necp_stat_txpackets             __attribute__((aligned(8)));
	u_int64_t       necp_stat_txbytes               __attribute__((aligned(8)));

	u_int32_t       necp_stat_rxduplicatebytes;
	u_int32_t       necp_stat_rxoutoforderbytes;
	u_int32_t       necp_stat_txretransmit;

	u_int32_t       necp_stat_connectattempts;
	u_int32_t       necp_stat_connectsuccesses;

	u_int32_t       necp_stat_min_rtt;
	u_int32_t       necp_stat_avg_rtt;
	u_int32_t       necp_stat_var_rtt;

#define NECP_STAT_ROUTE_FLAGS   1
	u_int32_t       necp_stat_route_flags;
};

                                                             
                                                        
                                                                                             
  
                                                                        
                                                 
struct necp_basic_metadata {
	u_int32_t       rcvbufsize;
	u_int32_t       rcvbufused;
};

struct necp_connection_probe_status {
	unsigned int    probe_activated : 1;
	unsigned int    write_probe_failed : 1;
	unsigned int    read_probe_failed : 1;
	unsigned int    conn_probe_failed : 1;
};

struct necp_extra_tcp_metadata {
	struct necp_connection_probe_status probestatus;

	u_int32_t       sndbufsize;
	u_int32_t       sndbufused;
	u_int32_t       txunacked;
	u_int32_t       txwindow;
	u_int32_t       txcwindow;
	u_int32_t       flags;                              
	u_int32_t       flags1;                              
	u_int32_t       traffic_mgt_flags;
	u_int32_t       cc_alg_index;
	u_int32_t       state;
};

struct necp_stats_hdr {
	u_int32_t                                       necp_stats_type __attribute__((aligned(8)));
	u_int32_t                                       necp_stats_ver;
	u_int64_t                                       __necp_stats_reserved;                                
};

#define NECP_CLIENT_STATISTICS_TYPE_TCP                         1                                          
#define NECP_CLIENT_STATISTICS_TYPE_UDP                         2                                          
#define NECP_CLIENT_STATISTICS_TYPE_QUIC                        3                                           

#define NECP_CLIENT_STATISTICS_TYPE_TCP_VER_1           1                                             
#define NECP_CLIENT_STATISTICS_TYPE_UDP_VER_1           1                                             
#define NECP_CLIENT_STATISTICS_TYPE_QUIC_VER_1          1                                              

#define NECP_CLIENT_STATISTICS_TYPE_TCP_CURRENT_VER             NECP_CLIENT_STATISTICS_TYPE_TCP_VER_1
#define NECP_CLIENT_STATISTICS_TYPE_UDP_CURRENT_VER             NECP_CLIENT_STATISTICS_TYPE_UDP_VER_1
#define NECP_CLIENT_STATISTICS_TYPE_QUIC_CURRENT_VER            NECP_CLIENT_STATISTICS_TYPE_QUIC_VER_1

#define NECP_CLIENT_STATISTICS_EVENT_INIT                       0x00000000                                  
#define NECP_CLIENT_STATISTICS_EVENT_TIME_WAIT          0x00000001                                                                      

struct necp_tcp_stats {
	struct necp_stats_hdr                   necp_tcp_hdr;
	struct necp_stat_counts                 necp_tcp_counts;
	struct necp_basic_metadata              necp_tcp_basic;
	struct necp_extra_tcp_metadata  necp_tcp_extra;
};

struct necp_udp_stats {
	struct necp_stats_hdr           necp_udp_hdr;
	struct necp_stat_counts         necp_udp_counts;
	struct necp_basic_metadata      necp_udp_basic;
};


  
                                                    
                                                  
                                                      
                             
                                                       
                                                                
                                                          
   
#define QUIC_STATELESS_RESET_TOKEN_SIZE               16
struct necp_extra_quic_metadata {
	u_int32_t       sndbufsize;
	u_int32_t       sndbufused;
	u_int32_t       txunacked;
	u_int32_t       txwindow;
	u_int32_t       txcwindow;
	u_int32_t       traffic_mgt_flags;
	u_int32_t       cc_alg_index;
	u_int32_t       state;
	u_int8_t        ssr_token[QUIC_STATELESS_RESET_TOKEN_SIZE];
};

#define necp_quic_hdr           necp_quic_udp_stats.necp_udp_hdr
#define necp_quic_counts        necp_quic_udp_stats.necp_udp_counts
#define necp_quic_basic         necp_quic_udp_stats.necp_udp_basic
struct necp_quic_stats {
	struct necp_udp_stats           necp_quic_udp_stats;
	struct necp_extra_quic_metadata necp_quic_extra;
};

typedef struct necp_all_stats {
	union {
		struct necp_tcp_stats   tcp_stats;
		struct necp_udp_stats   udp_stats;
		struct necp_quic_stats  quic_stats;
	} all_stats_u;
} necp_all_stats;

                                                             
  
struct necp_stats_bufreq {
	u_int32_t                                       necp_stats_bufreq_id __attribute__((aligned(8)));
	u_int32_t                                       necp_stats_bufreq_type;                                          
	u_int32_t                                       necp_stats_bufreq_ver;                                               
	u_int32_t                                       necp_stats_bufreq_size;
	union {
		void                                    *necp_stats_bufreq_addr;
		mach_vm_address_t               necp_stats_bufreq_uaddr;
	};
};

#define NECP_CLIENT_STATISTICS_BUFREQ_ID                                0xbf                                                                           

                                                                                                               
  
#define NECP_MAX_PER_PROCESS_CLIENT_STATISTICS_STRUCTS  512

#define NECP_TCP_ECN_HEURISTICS_SYN_RST 1
typedef struct necp_tcp_ecn_cache {
	u_int8_t                necp_tcp_ecn_heuristics_success:1;
	u_int8_t                necp_tcp_ecn_heuristics_loss:1;
	u_int8_t                necp_tcp_ecn_heuristics_drop_rst:1;
	u_int8_t                necp_tcp_ecn_heuristics_drop_rxmt:1;
	u_int8_t                necp_tcp_ecn_heuristics_aggressive:1;
	u_int8_t                necp_tcp_ecn_heuristics_syn_rst:1;
} necp_tcp_ecn_cache;

#define NECP_TCP_TFO_HEURISTICS_RST 1
typedef struct necp_tcp_tfo_cache {
	u_int8_t                necp_tcp_tfo_cookie[NECP_TFO_COOKIE_LEN_MAX];
	u_int8_t                necp_tcp_tfo_cookie_len;
	u_int8_t                necp_tcp_tfo_heuristics_success:1;                                      
	u_int8_t                necp_tcp_tfo_heuristics_loss:1;                          
	u_int8_t                necp_tcp_tfo_heuristics_middlebox:1;                          
	u_int8_t                necp_tcp_tfo_heuristics_success_req:1;                                                
	u_int8_t                necp_tcp_tfo_heuristics_loss_req:1;                                    
	u_int8_t                necp_tcp_tfo_heuristics_rst_data:1;                                              
	u_int8_t                necp_tcp_tfo_heuristics_rst_req:1;                                             
} necp_tcp_tfo_cache;

#define NECP_CLIENT_CACHE_TYPE_ECN                 1                                              
#define NECP_CLIENT_CACHE_TYPE_TFO                 2                                              

#define NECP_CLIENT_CACHE_TYPE_ECN_VER_1           1                                             
#define NECP_CLIENT_CACHE_TYPE_TFO_VER_1           1                                             

typedef struct necp_cache_buffer {
	u_int8_t                necp_cache_buf_type;                                
	u_int8_t                necp_cache_buf_ver;                                     
	u_int32_t               necp_cache_buf_size;
	mach_vm_address_t       necp_cache_buf_addr;
} necp_cache_buffer;

  
                          
   
#define NECP_MAX_CLIENT_PARAMETERS_SIZE                                 1024
#define NECP_MAX_CLIENT_RESULT_SIZE                                             512

#define NECP_OPEN_FLAG_OBSERVER                                                 0x01                                              
#define NECP_OPEN_FLAG_BACKGROUND                                               0x02                                
#define NECP_OPEN_FLAG_PUSH_OBSERVER                                    0x04                                                                                                              

#define NECP_FD_SUPPORTS_GUARD                                                  1

#define NECP_CLIENT_ACTION_ADD                                                  1                                                                         
#define NECP_CLIENT_ACTION_REMOVE                                               2                                                                               
#define NECP_CLIENT_ACTION_COPY_PARAMETERS                              3                                                                          
#define NECP_CLIENT_ACTION_COPY_RESULT                                  4                                                                  
#define NECP_CLIENT_ACTION_COPY_LIST                                    5                                                                  
#define NECP_CLIENT_ACTION_REQUEST_NEXUS_INSTANCE               6                                                                                     
#define NECP_CLIENT_ACTION_AGENT                                                7                                                           
#define NECP_CLIENT_ACTION_COPY_AGENT                                   8                                                                  
#define NECP_CLIENT_ACTION_COPY_INTERFACE                               9                                                                                              
#define NECP_CLIENT_ACTION_SET_STATISTICS                               10              
#define NECP_CLIENT_ACTION_COPY_ROUTE_STATISTICS                11                                                                           
#define NECP_CLIENT_ACTION_AGENT_USE                                    12                                                                                                    
#define NECP_CLIENT_ACTION_MAP_SYSCTLS                                  13                                                                        
#define NECP_CLIENT_ACTION_UPDATE_CACHE                                 14                               
#define NECP_CLIENT_ACTION_COPY_CLIENT_UPDATE                   15                                                                                                                   
#define NECP_CLIENT_ACTION_COPY_UPDATED_RESULT                  16                                                                                  
#define NECP_CLIENT_ACTION_ADD_FLOW                                             17                                                                     
#define NECP_CLIENT_ACTION_REMOVE_FLOW                                  18                                                                       
#define NECP_CLIENT_ACTION_CLAIM                                       19                                                                            
#define NECP_CLIENT_ACTION_SIGN                                       20                                                                                                                  
#define NECP_CLIENT_ACTION_GET_INTERFACE_ADDRESS                       21                                                                                                                                   

#define NECP_CLIENT_PARAMETER_APPLICATION                               NECP_POLICY_CONDITION_APPLICATION                                      
#define NECP_CLIENT_PARAMETER_REAL_APPLICATION                  NECP_POLICY_CONDITION_REAL_APPLICATION                         
#define NECP_CLIENT_PARAMETER_DOMAIN                                    NECP_POLICY_CONDITION_DOMAIN
#define NECP_CLIENT_PARAMETER_ACCOUNT                                   NECP_POLICY_CONDITION_ACCOUNT
#define NECP_CLIENT_PARAMETER_PID                                               NECP_POLICY_CONDITION_PID                                                      
#define NECP_CLIENT_PARAMETER_UID                                               NECP_POLICY_CONDITION_UID                                                      
#define NECP_CLIENT_PARAMETER_BOUND_INTERFACE                   NECP_POLICY_CONDITION_BOUND_INTERFACE
#define NECP_CLIENT_PARAMETER_TRAFFIC_CLASS                             NECP_POLICY_CONDITION_TRAFFIC_CLASS
#define NECP_CLIENT_PARAMETER_IP_PROTOCOL                               NECP_POLICY_CONDITION_IP_PROTOCOL
#define NECP_CLIENT_PARAMETER_LOCAL_ADDRESS                             NECP_POLICY_CONDITION_LOCAL_ADDR
#define NECP_CLIENT_PARAMETER_REMOTE_ADDRESS                    NECP_POLICY_CONDITION_REMOTE_ADDR
#define NECP_CLIENT_PARAMETER_NEXUS_KEY                                 102

                                                               
#define NECP_CLIENT_PARAMETER_PROHIBIT_INTERFACE                100                                      
#define NECP_CLIENT_PARAMETER_PROHIBIT_IF_TYPE                  101                                                                
#define NECP_CLIENT_PARAMETER_PROHIBIT_AGENT                    102                                          
#define NECP_CLIENT_PARAMETER_PROHIBIT_AGENT_TYPE               103                                                          

                                                                              
#define NECP_CLIENT_PARAMETER_REQUIRE_IF_TYPE                   111                                                                
#define NECP_CLIENT_PARAMETER_REQUIRE_AGENT                             112                                          
#define NECP_CLIENT_PARAMETER_REQUIRE_AGENT_TYPE                113                                                          

                                                                                  
#define NECP_CLIENT_PARAMETER_PREFER_AGENT                              122                                          
#define NECP_CLIENT_PARAMETER_PREFER_AGENT_TYPE                 123                                                          

                                                                                      
#define NECP_CLIENT_PARAMETER_AVOID_AGENT                               124                                          
#define NECP_CLIENT_PARAMETER_AVOID_AGENT_TYPE                  125                                                          

                                            
#define NECP_CLIENT_PARAMETER_TRIGGER_AGENT                             130                                          
#define NECP_CLIENT_PARAMETER_ASSERT_AGENT                              131                                          
#define NECP_CLIENT_PARAMETER_UNASSERT_AGENT                    132                                          

#define NECP_CLIENT_PARAMETER_PARENT_ID                                                 150                       

#define NECP_CLIENT_PARAMETER_LOCAL_ENDPOINT                    200                                           
#define NECP_CLIENT_PARAMETER_REMOTE_ENDPOINT                   201                                           
#define NECP_CLIENT_PARAMETER_BROWSE_DESCRIPTOR                  202                                           
#define NECP_CLIENT_PARAMETER_RESOLVER_TAG                      203                                                                     
#define NECP_CLIENT_PARAMETER_ADVERTISE_DESCRIPTOR                  204                                           

#define NECP_CLIENT_PARAMETER_DELEGATED_UPID                              210                                   

#define NECP_CLIENT_PARAMETER_ETHERTYPE                              220                        
#define NECP_CLIENT_PARAMETER_TRANSPORT_PROTOCOL                        221                      

#define NECP_CLIENT_PARAMETER_LOCAL_ADDRESS_PREFERENCE                        230                                                             

#define NECP_CLIENT_PARAMETER_LOCAL_ADDRESS_PREFERENCE_DEFAULT                    0
#define NECP_CLIENT_PARAMETER_LOCAL_ADDRESS_PREFERENCE_TEMPORARY                    1
#define NECP_CLIENT_PARAMETER_LOCAL_ADDRESS_PREFERENCE_STABLE                    2

#define NECP_CLIENT_PARAMETER_FLAGS                                             250                                                                  

#define NECP_CLIENT_PARAMETER_FLAG_MULTIPATH                    0x0001                                    
#define NECP_CLIENT_PARAMETER_FLAG_BROWSE                               0x0002                                                       
#define NECP_CLIENT_PARAMETER_FLAG_PROHIBIT_EXPENSIVE   0x0004                                  
#define NECP_CLIENT_PARAMETER_FLAG_LISTENER                             0x0008                                                              
#define NECP_CLIENT_PARAMETER_FLAG_DISCRETIONARY                0x0010                                                                         
#define NECP_CLIENT_PARAMETER_FLAG_ECN_ENABLE                   0x0020                                       
#define NECP_CLIENT_PARAMETER_FLAG_ECN_DISABLE                  0x0040                                        
#define NECP_CLIENT_PARAMETER_FLAG_TFO_ENABLE                   0x0080                                       
#define NECP_CLIENT_PARAMETER_FLAG_ONLY_PRIMARY_REQUIRES_TYPE   0x0100                                                                                                                                  
#define NECP_CLIENT_PARAMETER_FLAG_CUSTOM_ETHER                 0x0200                                                     
#define NECP_CLIENT_PARAMETER_FLAG_CUSTOM_IP                    0x0400                                                        
#define NECP_CLIENT_PARAMETER_FLAG_INTERPOSE                    0x0800                                                       
#define NECP_CLIENT_PARAMETER_FLAG_PROHIBIT_CONSTRAINED       0x1000                                    
#define NECP_CLIENT_PARAMETER_FLAG_FALLBACK_TRAFFIC           0x2000                     
#define NECP_CLIENT_PARAMETER_FLAG_INBOUND                    0x4000                              

#define NECP_CLIENT_RESULT_CLIENT_ID                                    1                        
#define NECP_CLIENT_RESULT_POLICY_RESULT                                2                           
#define NECP_CLIENT_RESULT_POLICY_RESULT_PARAMETER              3                           
#define NECP_CLIENT_RESULT_FILTER_CONTROL_UNIT                  4                           
#define NECP_CLIENT_RESULT_INTERFACE_INDEX                              5                           
#define NECP_CLIENT_RESULT_NETAGENT                                             6                                                    
#define NECP_CLIENT_RESULT_FLAGS                                                7                                                                 
#define NECP_CLIENT_RESULT_INTERFACE                                    8                                                     
#define NECP_CLIENT_RESULT_INTERFACE_OPTION                             9                                                     
#define NECP_CLIENT_RESULT_EFFECTIVE_MTU                                10                          
#define NECP_CLIENT_RESULT_FLOW                                                 11                                                   
#define NECP_CLIENT_RESULT_PROTO_CTL_EVENT                              12
#define NECP_CLIENT_RESULT_TFO_COOKIE                                   13                                        
#define NECP_CLIENT_RESULT_TFO_FLAGS                                    14                         
#define NECP_CLIENT_RESULT_RECOMMENDED_MSS                              15                         
#define NECP_CLIENT_RESULT_FLOW_ID                                              16                       
#define NECP_CLIENT_RESULT_INTERFACE_TIME_DELTA                 17                                                           
#define NECP_CLIENT_RESULT_REASON                                               18                                                                  
#define NECP_CLIENT_RESULT_FLOW_DIVERT_AGGREGATE_UNIT                   19                          

#define NECP_CLIENT_RESULT_NEXUS_INSTANCE                               100                      
#define NECP_CLIENT_RESULT_NEXUS_PORT                                   101                         
#define NECP_CLIENT_RESULT_NEXUS_KEY                                    102                      
#define NECP_CLIENT_RESULT_NEXUS_PORT_FLOW_INDEX                103                         
#define NECP_CLIENT_RESULT_NEXUS_FLOW_STATS                             104                                      

#define NECP_CLIENT_RESULT_LOCAL_ENDPOINT                               200                                           
#define NECP_CLIENT_RESULT_REMOTE_ENDPOINT                              201                                           
#define NECP_CLIENT_RESULT_DISCOVERED_ENDPOINT                  202                                                             
#define NECP_CLIENT_RESULT_RESOLVED_ENDPOINT                  203                                                              
#define NECP_CLIENT_RESULT_LOCAL_ETHER_ADDR                                     204                                         
#define NECP_CLIENT_RESULT_REMOTE_ETHER_ADDR                                    205                                         
#define NECP_CLIENT_RESULT_EFFECTIVE_TRAFFIC_CLASS              210                         
#define NECP_CLIENT_RESULT_TRAFFIC_MGMT_BG                              211                                                           
#define NECP_CLIENT_RESULT_GATEWAY                                      212                                           
#define NECP_CLIENT_RESULT_NAT64                                        213                                                          

#define NECP_CLIENT_RESULT_FLAG_IS_LOCAL                                0x0001                          
#define NECP_CLIENT_RESULT_FLAG_IS_DIRECT                               0x0002                                       
#define NECP_CLIENT_RESULT_FLAG_HAS_IPV4                                0x0004                  
#define NECP_CLIENT_RESULT_FLAG_HAS_IPV6                                0x0008                  
#define NECP_CLIENT_RESULT_FLAG_DEFUNCT                                 0x0010            
#define NECP_CLIENT_RESULT_FLAG_SATISFIED                               0x0020                   
#define NECP_CLIENT_RESULT_FLAG_FLOW_ASSIGNED                   0x0040                                 
#define NECP_CLIENT_RESULT_FLAG_FLOW_VIABLE                             0x0080                                       
#define NECP_CLIENT_RESULT_FLAG_PROBE_CONNECTIVITY              0x0100                                   
#define NECP_CLIENT_RESULT_FLAG_ECN_ENABLED                             0x0200                       
#define NECP_CLIENT_RESULT_FLAG_FAST_OPEN_BLOCKED               0x0400                                 
#define NECP_CLIENT_RESULT_FLAG_LINK_QUALITY_ABORT              0x0800                                                          
#define NECP_CLIENT_RESULT_FLAG_ALLOW_QOS_MARKING               0x1000                           
#define NECP_CLIENT_RESULT_FLAG_HAS_NAT64                       0x2000                     
#define NECP_CLIENT_RESULT_FLAG_INTERFACE_LOW_POWER             0x4000                                   
#define NECP_CLIENT_RESULT_FLAG_SPECIFIC_LISTENER               0x8000                                                 

#define NECP_CLIENT_RESULT_FLAG_FORCE_UPDATE (NECP_CLIENT_RESULT_FLAG_HAS_IPV4 | NECP_CLIENT_RESULT_FLAG_HAS_IPV6 | NECP_CLIENT_RESULT_FLAG_HAS_NAT64 | NECP_CLIENT_RESULT_FLAG_INTERFACE_LOW_POWER)

#define NECP_CLIENT_RESULT_FAST_OPEN_SND_PROBE                  0x01                                        
#define NECP_CLIENT_RESULT_FAST_OPEN_RCV_PROBE                  0x02                                           

#define NECP_CLIENT_RESULT_RECOMMENDED_MSS_NONE                 0x01
#define NECP_CLIENT_RESULT_RECOMMENDED_MSS_LOW                  0x02
#define NECP_CLIENT_RESULT_RECOMMENDED_MSS_MEDIUM               0x04

#define NECP_CLIENT_RESULT_REASON_EXPENSIVE_PROHIBITED          1                                       
#define NECP_CLIENT_RESULT_REASON_CONSTRAINED_PROHIBITED                2                                         
#define NECP_CLIENT_RESULT_REASON_CELLULAR_DENIED                3                                    
#define NECP_CLIENT_RESULT_REASON_WIFI_DENIED                4                                
#define NECP_CLIENT_RESULT_REASON_LOCAL_NETWORK_PROHIBITED       5                                    

struct necp_interface_signature {
	u_int8_t signature[IFNET_SIGNATURELEN];
	u_int8_t signature_len;
};

struct necp_interface_details {
	char name[IFXNAMSIZ];
	u_int32_t index;
	u_int32_t generation;
	u_int32_t functional_type;
	u_int32_t delegate_index;
	u_int32_t flags;                             
	u_int32_t mtu;
	struct necp_interface_signature ipv4_signature;
	struct necp_interface_signature ipv6_signature;
	u_int32_t ipv4_netmask;
	u_int32_t ipv4_broadcast;
};

#define NECP_INTERFACE_FLAG_EXPENSIVE                                   0x0001
#define NECP_INTERFACE_FLAG_TXSTART                                     0X0002
#define NECP_INTERFACE_FLAG_NOACKPRI                                    0x0004
#define NECP_INTERFACE_FLAG_3CARRIERAGG                                 0x0008
#define NECP_INTERFACE_FLAG_IS_LOW_POWER                                0x0010
#define NECP_INTERFACE_FLAG_MPK_LOG                                     0x0020                              
#define NECP_INTERFACE_FLAG_CONSTRAINED                                 0x0040
#define NECP_INTERFACE_FLAG_HAS_NETMASK                                 0x0080
#define NECP_INTERFACE_FLAG_HAS_BROADCAST                               0x0100
#define NECP_INTERFACE_FLAG_SUPPORTS_MULTICAST                          0x0200
#define NECP_INTERFACE_FLAG_HAS_DNS                                     0x0400
#define NECP_INTERFACE_FLAG_HAS_NAT64                                   0x0800
#define NECP_INTERFACE_FLAG_IPV4_ROUTABLE                               0x1000
#define NECP_INTERFACE_FLAG_IPV6_ROUTABLE                               0x2000

struct necp_client_parameter_netagent_type {
	char netagent_domain[32];
	char netagent_type[32];
};

struct necp_client_result_netagent {
	u_int32_t generation;
	uuid_t netagent_uuid;
};

struct necp_client_result_interface {
	u_int32_t generation;
	u_int32_t index;
};

#define NECP_USES_INTERFACE_OPTIONS_FOR_BROWSE 1

struct necp_client_interface_option {
	u_int32_t interface_index;
	u_int32_t interface_generation;
	uuid_t nexus_agent;
};

struct necp_client_endpoint {
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
		struct {
			u_int8_t endpoint_length;
			u_int8_t endpoint_family;                                  
			u_int16_t endpoint_port;
			u_int32_t endpoint_type;                        
			char endpoint_data[0];                                
		} endpoint;
	} u;
};

struct necp_client_list {
	u_int32_t client_count;
	uuid_t clients[0];
};

struct kev_necp_policies_changed_data {
	u_int32_t               changed_count;                   
};

#define NECP_CLIENT_FLOW_FLAGS_ALLOW_NEXUS                      0x01                                                  
#define NECP_CLIENT_FLOW_FLAGS_USE_CLIENT_ID            0x02                                                                                      
#define NECP_CLIENT_FLOW_FLAGS_BROWSE                      0x04                                         
#define NECP_CLIENT_FLOW_FLAGS_RESOLVE                      0x08                                             
#define NECP_CLIENT_FLOW_FLAGS_OVERRIDE_ADDRESS                      0x10                                                               

struct necp_client_flow_stats {
	u_int32_t stats_type;                                 
	u_int32_t stats_version;                                     
	u_int32_t stats_size;
	mach_vm_address_t stats_addr;
};

struct necp_client_add_flow {
	uuid_t agent_uuid;
	uuid_t registration_id;
	u_int16_t flags;                            
	u_int16_t stats_request_count;
	struct necp_client_flow_stats stats_requests[0];
	                                 
} __attribute__((__packed__));

struct necp_agent_use_parameters {
	uuid_t agent_uuid;
	uint64_t out_use_count;
};

struct necp_client_flow_protoctl_event {
	uint32_t        protoctl_event_code;
	uint32_t        protoctl_event_val;
	                                          
	uint32_t        protoctl_event_tcp_seq_num;
};

#define NECP_CLIENT_UPDATE_TYPE_PARAMETERS              1                                      
#define NECP_CLIENT_UPDATE_TYPE_RESULT                  2                                      
#define NECP_CLIENT_UPDATE_TYPE_REMOVE                  3                                     

struct necp_client_observer_update {
	u_int32_t update_type;                              
	u_int8_t tlv_buffer[0];                                               
};

#define NECP_CLIENT_SIGN_TYPE_RESOLVER_ANSWER           1

struct necp_client_signable {
	uuid_t client_id;
	u_int32_t sign_type;
} __attribute__((__packed__));

struct necp_client_resolver_answer {
	uuid_t client_id;
	u_int32_t sign_type;
	union sockaddr_in_4_6 address_answer;
	u_int32_t hostname_length;
	           
} __attribute__((__packed__));

#define NECP_FILTER_UNIT_NO_FILTER              UINT32_MAX                                                                            

  
                                                                                      
                                                                                      
                                 
   
#define MAX_NECP_DROP_DEST_LEVEL_ADDRS 8

struct necp_drop_dest_entry {
	u_int32_t                           level;                           
	u_int32_t                           order;                                                 
	struct necp_policy_condition_addr   cond_addr;
};

struct necp_drop_dest_policy {
	u_int32_t entry_count;
	struct necp_drop_dest_entry entries[MAX_NECP_DROP_DEST_LEVEL_ADDRS];
};

#ifdef BSD_KERNEL_PRIVATE
#include <stdbool.h>
#include <sys/socketvar.h>
#include <sys/kern_control.h>
#include <netinet/ip_var.h>
#include <netinet6/ip6_var.h>
#include <net/if_var.h>
#include <sys/syslog.h>
#include <net/network_agent.h>
#include <net/ethernet.h>
#include <os/log.h>


SYSCTL_DECL(_net_necp);

extern os_log_t necp_log_handle;

#define NECPLOG(level, format, ...) do {                                                                                        \
	if (level == LOG_ERR) {                                                          \
	    os_log_error(necp_log_handle, "%s: " format "\n", __FUNCTION__, __VA_ARGS__); \
	} else {                                                                         \
	    os_log(necp_log_handle, "%s: " format "\n", __FUNCTION__, __VA_ARGS__);       \
	}                                                                                \
} while (0)

#define NECPLOG0(level, msg) do {                                                                                       \
	if (level == LOG_ERR) {                                                          \
	    os_log_error(necp_log_handle, "%s: %s\n", __FUNCTION__, msg);                 \
	} else {                                                                         \
	    os_log(necp_log_handle, "%s: %s\n", __FUNCTION__, msg);                       \
	}                                                                                \
} while (0)

enum necp_fd_type_t {
	necp_fd_type_invalid = 0,
	necp_fd_type_session = 1,
	necp_fd_type_client = 2,
};

union necp_sockaddr_union {
	struct sockaddr                 sa;
	struct sockaddr_in              sin;
	struct sockaddr_in6             sin6;
};

  
         
                                                                      
                                                                            
                                                                           
                
   
struct necp_all_kstats {
	struct necp_all_stats           necp_stats_comm;                                           
	struct necp_all_stats           *necp_stats_ustats;                                                                 
};

extern errno_t necp_client_init(void);
extern int necp_application_find_policy_match_internal(proc_t proc, u_int8_t *parameters, u_int32_t parameters_size,
    struct necp_aggregate_result *returned_result,
    u_int32_t *flags, u_int32_t *reason, u_int required_interface_index,
    const union necp_sockaddr_union *override_local_addr,
    const union necp_sockaddr_union *override_remote_addr,
    struct necp_client_endpoint *returned_v4_gateway,
    struct necp_client_endpoint *returned_v6_gateway,
    struct rtentry **returned_route, bool ignore_address,
    bool has_client,
    uuid_t *returned_override_euuid);
  
                
  
                                                                                                            
                                                                   
   
struct necp_tlv_header {
	u_int8_t type;
	u_int32_t length;
} __attribute__((__packed__));

extern u_int8_t *necp_buffer_write_tlv(u_int8_t *cursor, u_int8_t type, u_int32_t length, const void *value,
    u_int8_t *buffer, u_int32_t buffer_length);
extern u_int8_t *necp_buffer_write_tlv_if_different(u_int8_t *cursor, u_int8_t type,
    u_int32_t length, const void *value, bool *updated,
    u_int8_t *buffer, u_int32_t buffer_length);
extern u_int8_t necp_buffer_get_tlv_type(u_int8_t *buffer, int tlv_offset);
extern u_int32_t necp_buffer_get_tlv_length(u_int8_t *buffer, int tlv_offset);
extern u_int8_t *necp_buffer_get_tlv_value(u_int8_t *buffer, int tlv_offset, u_int32_t *value_size);
extern int necp_buffer_find_tlv(u_int8_t *buffer, u_int32_t buffer_length, int offset, u_int8_t type, int *err, int next);

#define NECPCTL_DROP_ALL_LEVEL                          1                                                                   
#define NECPCTL_DEBUG                                           2                                          
#define NECPCTL_PASS_LOOPBACK                           3                                      
#define NECPCTL_PASS_KEEPALIVES                         4                                                        
#define NECPCTL_SOCKET_POLICY_COUNT                     5                                               
#define NECPCTL_SOCKET_NON_APP_POLICY_COUNT     6                                                       
#define NECPCTL_IP_POLICY_COUNT                         7                                           
#define NECPCTL_SESSION_COUNT                           8                                   
#define NECPCTL_CLIENT_FD_COUNT                         9                                     
#define NECPCTL_CLIENT_COUNT                            10                                 
#define NECPCTL_ARENA_COUNT                                     11                                             
#define NECPCTL_NEXUS_FLOW_COUNT                        12                                     
#define NECPCTL_SOCKET_FLOW_COUNT                       13                                      
#define NECPCTL_IF_FLOW_COUNT                           14                                      
#define NECPCTL_OBSERVER_FD_COUNT                       15                                      
#define NECPCTL_OBSERVER_MESSAGE_LIMIT          16                                                                    
#define NECPCTL_SYSCTL_ARENA_COUNT                      17                                  
#define NECPCTL_DROP_UNENTITLED_LEVEL                   18                                                            
#define NECPCTL_PASS_INTERPOSE                          19                          
#define NECPCTL_RESTRICT_MULTICAST                      20                                     
#define NECPCTL_DEDUP_POLICIES                          21                                      

#define NECP_LOOPBACK_PASS_ALL         1                              
#define NECP_LOOPBACK_PASS_WITH_FILTER 2                                                                                            

#define NECPCTL_NAMES {                                 \
	{ 0, 0 },                                                       \
	{ "drop_all_level", CTLTYPE_INT },      \
	{ "debug", CTLTYPE_INT },                       \
	{ "pass_loopback", CTLTYPE_INT },       \
    { "pass_keepalives", CTLTYPE_INT },     \
    { "pass_interpose", CTLTYPE_INT },      \
}

typedef u_int32_t necp_kernel_policy_id;
#define NECP_KERNEL_POLICY_ID_NONE                      0
#define NECP_KERNEL_POLICY_ID_NO_MATCH          1
#define NECP_KERNEL_POLICY_ID_FIRST_VALID_SOCKET        2
#define NECP_KERNEL_POLICY_ID_FIRST_VALID_IP            UINT16_MAX

typedef u_int32_t necp_app_id;

#define NECP_KERNEL_POLICY_RESULT_NONE                                  0
#define NECP_KERNEL_POLICY_RESULT_PASS                                  NECP_POLICY_RESULT_PASS
#define NECP_KERNEL_POLICY_RESULT_SKIP                                  NECP_POLICY_RESULT_SKIP
#define NECP_KERNEL_POLICY_RESULT_DROP                                  NECP_POLICY_RESULT_DROP
#define NECP_KERNEL_POLICY_RESULT_SOCKET_DIVERT                 NECP_POLICY_RESULT_SOCKET_DIVERT
#define NECP_KERNEL_POLICY_RESULT_SOCKET_FILTER                 NECP_POLICY_RESULT_SOCKET_FILTER
#define NECP_KERNEL_POLICY_RESULT_IP_TUNNEL                             NECP_POLICY_RESULT_IP_TUNNEL
#define NECP_KERNEL_POLICY_RESULT_IP_FILTER                             NECP_POLICY_RESULT_IP_FILTER
#define NECP_KERNEL_POLICY_RESULT_TRIGGER                               NECP_POLICY_RESULT_TRIGGER
#define NECP_KERNEL_POLICY_RESULT_TRIGGER_IF_NEEDED             NECP_POLICY_RESULT_TRIGGER_IF_NEEDED
#define NECP_KERNEL_POLICY_RESULT_TRIGGER_SCOPED                NECP_POLICY_RESULT_TRIGGER_SCOPED
#define NECP_KERNEL_POLICY_RESULT_NO_TRIGGER_SCOPED             NECP_POLICY_RESULT_NO_TRIGGER_SCOPED
#define NECP_KERNEL_POLICY_RESULT_SOCKET_SCOPED                 NECP_POLICY_RESULT_SOCKET_SCOPED
#define NECP_KERNEL_POLICY_RESULT_ROUTE_RULES                   NECP_POLICY_RESULT_ROUTE_RULES
#define NECP_KERNEL_POLICY_RESULT_USE_NETAGENT                  NECP_POLICY_RESULT_USE_NETAGENT
#define NECP_KERNEL_POLICY_RESULT_NETAGENT_SCOPED               NECP_POLICY_RESULT_NETAGENT_SCOPED
#define NECP_KERNEL_POLICY_RESULT_SCOPED_DIRECT                 NECP_POLICY_RESULT_SCOPED_DIRECT
#define NECP_KERNEL_POLICY_RESULT_ALLOW_UNENTITLED              NECP_POLICY_RESULT_ALLOW_UNENTITLED

#define NECP_KERNEL_POLICY_PASS_NO_SKIP_IPSEC                   NECP_POLICY_PASS_NO_SKIP_IPSEC
#define NECP_KERNEL_POLICY_PASS_PF_TAG                          NECP_POLICY_PASS_PF_TAG

#define NECP_KERNEL_POLICY_DROP_FLAG_LOCAL_NETWORK              NECP_POLICY_DROP_FLAG_LOCAL_NETWORK

typedef struct {
	u_int32_t identifier;
	u_int32_t data;
} necp_kernel_policy_service;

typedef union {
	u_int                                           tunnel_interface_index;
	u_int                                           scoped_interface_index;
	u_int32_t                                       flow_divert_control_unit;
	u_int32_t                                       filter_control_unit;
	u_int32_t                                       skip_policy_order;
	u_int32_t                                       route_rule_id;
	u_int32_t                                       netagent_id;
	u_int32_t                                       pass_flags;
	u_int32_t                                       drop_flags;
	necp_kernel_policy_service      service;
} necp_kernel_policy_result_parameter;

enum necp_boolean_state {
	necp_boolean_state_unknown = 0,
	necp_boolean_state_false = 1,
	necp_boolean_state_true = 2,
};

struct necp_kernel_socket_policy {
	LIST_ENTRY(necp_kernel_socket_policy)   chain;
	necp_kernel_policy_id           id;
	necp_policy_order                       order;
	u_int32_t                                       session_order;
	int                                                     session_pid;

	u_int32_t                                       condition_mask;
	u_int32_t                                       condition_negated_mask;
	u_int32_t                                       cond_client_flags;
	necp_kernel_policy_id           cond_policy_id;
	u_int32_t                                       cond_app_id;                                                                       
	u_int32_t                                       cond_real_app_id;                                                                  
	char                                            *cond_custom_entitlement;                        
	u_int8_t                                        cond_custom_entitlement_matched;                                     
	u_int32_t                                       cond_account_id;                                                                   
	char                                            *cond_domain;                                            
	u_int8_t                                        cond_domain_dot_count;                                                  
	pid_t                                           cond_pid;
	uid_t                                           cond_uid;
	ifnet_t                                         cond_bound_interface;                                                   
	struct necp_policy_condition_tc_range cond_traffic_class;                                        
	u_int16_t                                       cond_protocol;                                                              
	union necp_sockaddr_union       cond_local_start;                                                                     
	union necp_sockaddr_union       cond_local_end;                                                            
	u_int8_t                                        cond_local_prefix;                                               
	union necp_sockaddr_union       cond_remote_start;                                                                     
	union necp_sockaddr_union       cond_remote_end;                                                           
	u_int8_t                                        cond_remote_prefix;                                              
	struct necp_policy_condition_agent_type cond_agent_type;
	struct necp_policy_condition_sdk_version cond_sdk_version;
	char                                            *cond_signing_identifier;            
	u_int16_t                                       cond_packet_filter_tags;
	int32_t                                         cond_pid_version;

	necp_kernel_policy_result       result;
	necp_kernel_policy_result_parameter     result_parameter;
};

struct necp_kernel_ip_output_policy {
	LIST_ENTRY(necp_kernel_ip_output_policy)        chain;
	necp_kernel_policy_id           id;
	necp_policy_order                       suborder;
	necp_policy_order                       order;
	u_int32_t                                       session_order;
	int                                                     session_pid;

	u_int32_t                                       condition_mask;
	u_int32_t                                       condition_negated_mask;
	necp_kernel_policy_id           cond_policy_id;
	ifnet_t                                         cond_bound_interface;                                                   
	u_int16_t                                       cond_protocol;                                                              
	union necp_sockaddr_union       cond_local_start;                                                                     
	union necp_sockaddr_union       cond_local_end;                                                            
	u_int8_t                                        cond_local_prefix;                                               
	union necp_sockaddr_union       cond_remote_start;                                                                     
	union necp_sockaddr_union       cond_remote_end;                                                           
	u_int8_t                                        cond_remote_prefix;                                              
	u_int32_t                                       cond_last_interface_index;
	u_int16_t                       cond_packet_filter_tags;

	necp_kernel_policy_result       result;
	necp_kernel_policy_result_parameter     result_parameter;
};

#define MAX_KERNEL_SOCKET_POLICIES                      1
#define MAX_KERNEL_IP_OUTPUT_POLICIES           4
struct necp_session_policy {
	LIST_ENTRY(necp_session_policy) chain;
	bool                            applied;                                                        
	bool                            pending_deletion;                                                 
	bool                            pending_update;                                                                    
	necp_policy_id          local_id;
	necp_policy_order       order;
	u_int8_t                        *result;
	u_int32_t                       result_size;
	u_int8_t                        *conditions;                                                              
	u_int32_t                       conditions_size;
	u_int8_t                        *route_rules;                                                               
	u_int32_t                       route_rules_size;

	uuid_t                          applied_app_uuid;
	uuid_t                          applied_real_app_uuid;
	char                            *applied_account;

	uuid_t                          applied_result_uuid;

	u_int32_t                       applied_route_rules_id;

	necp_kernel_policy_id   kernel_socket_policies[MAX_KERNEL_SOCKET_POLICIES];
	necp_kernel_policy_id   kernel_ip_output_policies[MAX_KERNEL_IP_OUTPUT_POLICIES];
};

struct necp_aggregate_socket_result {
	necp_kernel_policy_result                       result;
	necp_kernel_policy_result_parameter     result_parameter;
	necp_kernel_policy_filter                       filter_control_unit;
	u_int32_t                                       flow_divert_aggregate_unit;
	u_int32_t                                                       route_rule_id;
	int32_t                                         qos_marking_gencount;
};

struct necp_inpcb_result {
	u_int32_t                                       app_id;
	necp_kernel_policy_id                           policy_id;
	necp_kernel_policy_id                           skip_policy_id;
	int32_t                                         policy_gencount;
	u_int32_t                                       flowhash;
	u_int32_t                                       network_denied_notifies;                     
	struct necp_aggregate_socket_result             results;
};

extern errno_t necp_init(void);

extern errno_t necp_set_socket_attributes(struct socket *so, struct sockopt *sopt);
extern errno_t necp_get_socket_attributes(struct socket *so, struct sockopt *sopt);
extern void necp_inpcb_remove_cb(struct inpcb *inp);
extern void necp_inpcb_dispose(struct inpcb *inp);

extern u_int32_t necp_socket_get_content_filter_control_unit(struct socket *so);

extern bool necp_socket_should_use_flow_divert(struct inpcb *inp);
extern u_int32_t necp_socket_get_flow_divert_control_unit(struct inpcb *inp, uint32_t *aggregate_unit);

extern bool necp_socket_should_rescope(struct inpcb *inp);
extern u_int necp_socket_get_rescope_if_index(struct inpcb *inp);
extern u_int32_t necp_socket_get_effective_mtu(struct inpcb *inp, u_int32_t current_mtu);

extern bool necp_socket_is_allowed_to_recv_on_interface(struct inpcb *inp, ifnet_t interface);

extern bool necp_socket_is_allowed_to_send_recv(struct inpcb *inp, ifnet_t input_interface, u_int16_t pf_tag,
    necp_kernel_policy_id *return_policy_id,
    u_int32_t *return_route_rule_id,
    necp_kernel_policy_id *return_skip_policy_id, u_int32_t *return_pass_flags);
extern bool necp_socket_is_allowed_to_send_recv_v4(struct inpcb *inp, u_int16_t local_port,
    u_int16_t remote_port, struct in_addr *local_addr,
    struct in_addr *remote_addr, ifnet_t input_interface, u_int16_t pf_tag,
    necp_kernel_policy_id *return_policy_id, u_int32_t *return_route_rule_id,
    necp_kernel_policy_id *return_skip_policy_id, u_int32_t *return_pass_flags);
extern bool necp_socket_is_allowed_to_send_recv_v6(struct inpcb *inp, u_int16_t local_port,
    u_int16_t remote_port, struct in6_addr *local_addr,
    struct in6_addr *remote_addr, ifnet_t input_interface, u_int16_t pf_tag,
    necp_kernel_policy_id *return_policy_id, u_int32_t *return_route_rule_id,
    necp_kernel_policy_id *return_skip_policy_id, u_int32_t *return_pass_flags);
extern void necp_socket_update_qos_marking(struct inpcb *inp, struct rtentry *route, u_int32_t route_rule_id);
extern bool necp_lookup_current_qos_marking(int32_t *qos_marking_gencount, struct rtentry *route, struct ifnet *interface, u_int32_t route_rule_id, bool old_qos_marking);
extern int necp_mark_packet_from_socket(struct mbuf *packet, struct inpcb *inp, necp_kernel_policy_id policy_id,
    u_int32_t route_rule_id, necp_kernel_policy_id skip_policy_id, u_int32_t pass_flags);
extern necp_kernel_policy_id necp_get_policy_id_from_packet(struct mbuf *packet);
extern necp_kernel_policy_id necp_get_skip_policy_id_from_packet(struct mbuf *packet);
extern u_int16_t necp_get_packet_filter_tags_from_packet(struct mbuf *packet);
extern bool necp_packet_should_skip_filters(struct mbuf *packet);
extern u_int32_t necp_get_last_interface_index_from_packet(struct mbuf *packet);
extern u_int32_t necp_get_route_rule_id_from_packet(struct mbuf *packet);
extern int necp_get_app_uuid_from_packet(struct mbuf *packet,
    uuid_t app_uuid);

extern necp_kernel_policy_id necp_socket_find_policy_match(struct inpcb *inp, struct sockaddr *override_local_addr,
    struct sockaddr *override_remote_addr, u_int32_t override_bound_interface);
extern necp_kernel_policy_id necp_ip_output_find_policy_match(struct mbuf *packet, int flags, struct ip_out_args *ipoa,
    struct rtentry *rt,
    necp_kernel_policy_result *result,
    necp_kernel_policy_result_parameter *result_parameter);
extern necp_kernel_policy_id necp_ip6_output_find_policy_match(struct mbuf *packet, int flags, struct ip6_out_args *ip6oa,
    struct rtentry *rt,
    necp_kernel_policy_result *result,
    necp_kernel_policy_result_parameter *result_parameter);

extern int necp_mark_packet_from_ip(struct mbuf *packet, necp_kernel_policy_id policy_id);
extern int necp_mark_packet_from_interface(struct mbuf *packet, ifnet_t interface);

extern ifnet_t necp_get_ifnet_from_result_parameter(necp_kernel_policy_result_parameter *result_parameter);
extern bool necp_packet_can_rebind_to_ifnet(struct mbuf *packet, struct ifnet *interface, struct route *new_route, int family);

extern bool necp_packet_is_allowed_over_interface(struct mbuf *packet, struct ifnet *interface);

extern int necp_mark_packet_as_keepalive(struct mbuf *packet, bool is_keepalive);
extern bool necp_get_is_keepalive_from_packet(struct mbuf *packet);

extern int necp_sign_resolver_answer(uuid_t client_id, u_int8_t *query, u_int32_t query_length,
    u_int8_t *answer, u_int32_t answer_length,
    u_int8_t *tag, u_int32_t *out_tag_length);

extern bool necp_validate_resolver_answer(uuid_t client_id, u_int8_t *query, u_int32_t query_length,
    u_int8_t *answer, u_int32_t answer_length,
    u_int8_t *tag, u_int32_t tag_length);

extern void necp_update_all_clients(void);                                    
extern void necp_update_all_clients_immediately_if_needed(bool should_update_immediately);                                    

extern void necp_force_update_client(uuid_t client_id, uuid_t remove_netagent_uuid, u_int32_t agent_generation);                                                

extern bool necp_set_client_as_background(proc_t proc, struct fileproc *fp, bool background);                                                  

struct necp_fd_data;
extern void necp_fd_memstatus(proc_t proc, uint32_t status, struct necp_fd_data *client_fd);                                           
extern void necp_fd_defunct(proc_t proc, struct necp_fd_data *client_fd);                                             

extern int necp_client_register_socket_flow(pid_t pid, uuid_t client_id, struct inpcb *inp);

extern int necp_client_register_socket_listener(pid_t pid, uuid_t client_id, struct inpcb *inp);


extern int necp_client_assert_bb_radio_manager(uuid_t client_id, bool assert);

extern int necp_client_assign_from_socket(pid_t pid, uuid_t client_id, struct inpcb *inp);

extern int necp_assign_client_result(uuid_t netagent_uuid, uuid_t client_id,
    u_int8_t *assigned_results, size_t assigned_results_length);
struct skmem_obj_info;                        
extern int necp_stats_ctor(struct skmem_obj_info *oi, struct skmem_obj_info *oim, void *arg, uint32_t skmflag);
extern int necp_stats_dtor(void *addr, void *arg);

                                                 
struct netagent_session;
extern int
necp_update_flow_protoctl_event(uuid_t netagent_uuid, uuid_t client_id,
    uint32_t protoctl_event_code, uint32_t protoctl_event_val,
    uint32_t protoctl_event_tcp_seq_num);

#define NECP_FLOWADV_IDX_INVALID        UINT32_MAX
extern void *necp_create_nexus_assign_message(uuid_t nexus_instance, u_int32_t nexus_port, void *key, uint32_t key_length,
    struct necp_client_endpoint *local_endpoint, struct necp_client_endpoint *remote_endpoint,
    struct ether_addr *local_ether_addr,
    u_int32_t flow_adv_index, void *flow_stats, size_t *message_length);

struct necp_client_nexus_parameters {
	pid_t pid;
	pid_t epid;
	uuid_t euuid;
	union necp_sockaddr_union local_addr;
	union necp_sockaddr_union remote_addr;
	u_int8_t ip_protocol;
	u_int8_t transport_protocol;
	u_int16_t ethertype;
	u_int32_t traffic_class;
	necp_policy_id policy_id;
	unsigned is_listener:1;
	unsigned is_interpose:1;
	unsigned is_custom_ether:1;
	unsigned allow_qos_marking:1;
	unsigned override_address_selection:1;
	unsigned use_stable_address:1;                                             
};

struct necp_client_agent_parameters {
	union {
		struct necp_client_nexus_parameters nexus_request;
		u_int8_t close_token[QUIC_STATELESS_RESET_TOKEN_SIZE];
	} u;
};

#define NECP_CLIENT_CBACTION_NONVIABLE  1
#define NECP_CLIENT_CBACTION_VIABLE     2
#define NECP_CLIENT_CBACTION_INITIAL    3

struct necp_client_add_flow_default {
	uuid_t agent_uuid;
	uuid_t registration_id;
	u_int16_t flags;                            
	u_int16_t stats_request_count;
	struct necp_client_flow_stats stats_requests[1];
} __attribute__((__packed__));

typedef void (*necp_client_flow_cb)(void *handle, int action, uint32_t interface_index, uint32_t necp_flags, bool *viable);

extern void necp_client_reap_caches(boolean_t purge);


#endif                         

#ifdef KERNEL
#ifdef KERNEL_PRIVATE
extern bool net_domain_contains_hostname(char *hostname_string, char *domain_string);
#endif                     
#endif             

#ifndef KERNEL

extern int necp_match_policy(const uint8_t *parameters, size_t parameters_size, struct necp_aggregate_result *returned_result);

extern int necp_open(int flags);

extern int necp_client_action(int necp_fd, uint32_t action, uuid_t client_id,
    size_t client_id_len, uint8_t *buffer, size_t buffer_size);

extern int necp_session_open(int flags);

extern int necp_session_action(int necp_fd, uint32_t action,
    uint8_t *in_buffer, size_t in_buffer_length,
    uint8_t *out_buffer, size_t out_buffer_length);

#endif              

#endif              

#endif
