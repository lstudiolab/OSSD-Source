/*
 * Copyright (c) 2010-2017 Apple Inc. All rights reserved.
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
#ifndef __NTSTAT_H__
#define __NTSTAT_H__
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/net_api_stats.h>
#include <netinet/in_stat.h>
#include <netinet/tcp.h>

#ifdef PRIVATE
#pragma mark -- Common Data Structures --

#define __NSTAT_REVISION__      9

typedef u_int32_t       nstat_provider_id_t;
typedef u_int64_t       nstat_src_ref_t;
typedef u_int64_t       nstat_event_flags_t;

                                                         
enum{
	NSTAT_EVENT_SRC_ADDED                    = 0x00000001
	, NSTAT_EVENT_SRC_REMOVED                = 0x00000002
	, NSTAT_EVENT_SRC_QUERIED                = 0x00000004
	, NSTAT_EVENT_SRC_QUERIED_ALL            = 0x00000008
	, NSTAT_EVENT_SRC_WILL_CHANGE_STATE      = 0x00000010
	, NSTAT_EVENT_SRC_DID_CHANGE_STATE       = 0x00000020
	, NSTAT_EVENT_SRC_WILL_CHANGE_OWNER      = 0x00000040
	, NSTAT_EVENT_SRC_DID_CHANGE_OWNER       = 0x00000080
	, NSTAT_EVENT_SRC_WILL_CHANGE_PROPERTY   = 0x00000100
	, NSTAT_EVENT_SRC_DID_CHANGE_PROPERTY    = 0x00000200
	, NSTAT_EVENT_SRC_ENTER_CELLFALLBACK     = 0x00000400
	, NSTAT_EVENT_SRC_EXIT_CELLFALLBACK      = 0x00000800
#if (DEBUG || DEVELOPMENT)
	, NSTAT_EVENT_SRC_RESERVED_1             = 0x00001000
	, NSTAT_EVENT_SRC_RESERVED_2             = 0x00002000
#endif                             
};

typedef struct nstat_counts {
	              
	u_int64_t       nstat_rxpackets __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_rxbytes   __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_txpackets __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_txbytes   __attribute__((aligned(sizeof(u_int64_t))));

	u_int64_t       nstat_cell_rxbytes      __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_cell_txbytes      __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_wifi_rxbytes      __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_wifi_txbytes      __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_wired_rxbytes     __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       nstat_wired_txbytes     __attribute__((aligned(sizeof(u_int64_t))));

	u_int32_t       nstat_rxduplicatebytes;
	u_int32_t       nstat_rxoutoforderbytes;
	u_int32_t       nstat_txretransmit;

	u_int32_t       nstat_connectattempts;
	u_int32_t       nstat_connectsuccesses;

	u_int32_t       nstat_min_rtt;
	u_int32_t       nstat_avg_rtt;
	u_int32_t       nstat_var_rtt;
} nstat_counts;

#define NSTAT_SYSINFO_KEYVAL_STRING_MAXSIZE     24
typedef struct nstat_sysinfo_keyval {
	u_int32_t       nstat_sysinfo_key;
	u_int32_t       nstat_sysinfo_flags;
	union {
		int64_t nstat_sysinfo_scalar;
		double  nstat_sysinfo_distribution;
		u_int8_t nstat_sysinfo_string[NSTAT_SYSINFO_KEYVAL_STRING_MAXSIZE];
	} u;
	u_int32_t       nstat_sysinfo_valsize;
	u_int8_t        reserved[4];
}  nstat_sysinfo_keyval;

#define NSTAT_SYSINFO_FLAG_SCALAR       0x0001
#define NSTAT_SYSINFO_FLAG_DISTRIBUTION 0x0002
#define NSTAT_SYSINFO_FLAG_STRING       0x0004

#define NSTAT_MAX_MSG_SIZE      4096

typedef struct nstat_sysinfo_counts {
	              
	u_int32_t       nstat_sysinfo_len;
	u_int32_t       pad;
	u_int8_t        nstat_sysinfo_keyvals[];
}  nstat_sysinfo_counts;

enum{
	NSTAT_SYSINFO_KEY_MBUF_256B_TOTAL        = 1
	, NSTAT_SYSINFO_KEY_MBUF_2KB_TOTAL       = 2
	, NSTAT_SYSINFO_KEY_MBUF_4KB_TOTAL       = 3
	, NSTAT_SYSINFO_KEY_SOCK_MBCNT           = 4
	, NSTAT_SYSINFO_KEY_SOCK_ATMBLIMIT       = 5
	, NSTAT_SYSINFO_KEY_IPV4_AVGRTT          = 6
	, NSTAT_SYSINFO_KEY_IPV6_AVGRTT          = 7
	, NSTAT_SYSINFO_KEY_SEND_PLR             = 8
	, NSTAT_SYSINFO_KEY_RECV_PLR             = 9
	, NSTAT_SYSINFO_KEY_SEND_TLRTO           = 10
	, NSTAT_SYSINFO_KEY_SEND_REORDERRATE     = 11
	, NSTAT_SYSINFO_CONNECTION_ATTEMPTS      = 12
	, NSTAT_SYSINFO_CONNECTION_ACCEPTS       = 13
	, NSTAT_SYSINFO_ECN_CLIENT_SETUP         = 14
	, NSTAT_SYSINFO_ECN_SERVER_SETUP         = 15
	, NSTAT_SYSINFO_ECN_CLIENT_SUCCESS       = 16
	, NSTAT_SYSINFO_ECN_SERVER_SUCCESS       = 17
	, NSTAT_SYSINFO_ECN_NOT_SUPPORTED        = 18
	, NSTAT_SYSINFO_ECN_LOST_SYN             = 19
	, NSTAT_SYSINFO_ECN_LOST_SYNACK          = 20
	, NSTAT_SYSINFO_ECN_RECV_CE              = 21
	, NSTAT_SYSINFO_ECN_RECV_ECE             = 22
	, NSTAT_SYSINFO_ECN_SENT_ECE             = 23
	, NSTAT_SYSINFO_ECN_CONN_RECV_CE         = 24
	, NSTAT_SYSINFO_ECN_CONN_PLNOCE          = 25
	, NSTAT_SYSINFO_ECN_CONN_PL_CE           = 26
	, NSTAT_SYSINFO_ECN_CONN_NOPL_CE         = 27
	, NSTAT_SYSINFO_MBUF_16KB_TOTAL          = 28
	, NSTAT_SYSINFO_ECN_CLIENT_ENABLED       = 29
	, NSTAT_SYSINFO_ECN_SERVER_ENABLED       = 30
	, NSTAT_SYSINFO_ECN_CONN_RECV_ECE        = 31
	, NSTAT_SYSINFO_MBUF_MEM_RELEASED        = 32
	, NSTAT_SYSINFO_MBUF_DRAIN_CNT           = 33
	, NSTAT_SYSINFO_TFO_SYN_DATA_RCV         = 34
	, NSTAT_SYSINFO_TFO_COOKIE_REQ_RCV       = 35
	, NSTAT_SYSINFO_TFO_COOKIE_SENT          = 36
	, NSTAT_SYSINFO_TFO_COOKIE_INVALID       = 37
	, NSTAT_SYSINFO_TFO_COOKIE_REQ           = 38
	, NSTAT_SYSINFO_TFO_COOKIE_RCV           = 39
	, NSTAT_SYSINFO_TFO_SYN_DATA_SENT        = 40
	, NSTAT_SYSINFO_TFO_SYN_DATA_ACKED       = 41
	, NSTAT_SYSINFO_TFO_SYN_LOSS             = 42
	, NSTAT_SYSINFO_TFO_BLACKHOLE            = 43
	, NSTAT_SYSINFO_ECN_FALLBACK_SYNLOSS     = 44
	, NSTAT_SYSINFO_ECN_FALLBACK_REORDER     = 45
	, NSTAT_SYSINFO_ECN_FALLBACK_CE          = 46
	, NSTAT_SYSINFO_ECN_IFNET_TYPE           = 47
	, NSTAT_SYSINFO_ECN_IFNET_PROTO          = 48
	, NSTAT_SYSINFO_ECN_IFNET_CLIENT_SETUP   = 49
	, NSTAT_SYSINFO_ECN_IFNET_SERVER_SETUP   = 50
	, NSTAT_SYSINFO_ECN_IFNET_CLIENT_SUCCESS = 51
	, NSTAT_SYSINFO_ECN_IFNET_SERVER_SUCCESS = 52
	, NSTAT_SYSINFO_ECN_IFNET_PEER_NOSUPPORT = 53
	, NSTAT_SYSINFO_ECN_IFNET_SYN_LOST       = 54
	, NSTAT_SYSINFO_ECN_IFNET_SYNACK_LOST    = 55
	, NSTAT_SYSINFO_ECN_IFNET_RECV_CE        = 56
	, NSTAT_SYSINFO_ECN_IFNET_RECV_ECE       = 57
	, NSTAT_SYSINFO_ECN_IFNET_SENT_ECE       = 58
	, NSTAT_SYSINFO_ECN_IFNET_CONN_RECV_CE   = 59
	, NSTAT_SYSINFO_ECN_IFNET_CONN_RECV_ECE  = 60
	, NSTAT_SYSINFO_ECN_IFNET_CONN_PLNOCE    = 61
	, NSTAT_SYSINFO_ECN_IFNET_CONN_PLCE      = 62
	, NSTAT_SYSINFO_ECN_IFNET_CONN_NOPLCE    = 63
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_SYNLOSS = 64
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_REORDER = 65
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_CE    = 66
	, NSTAT_SYSINFO_ECN_IFNET_ON_RTT_AVG     = 67
	, NSTAT_SYSINFO_ECN_IFNET_ON_RTT_VAR     = 68
	, NSTAT_SYSINFO_ECN_IFNET_ON_OOPERCENT   = 69
	, NSTAT_SYSINFO_ECN_IFNET_ON_SACK_EPISODE = 70
	, NSTAT_SYSINFO_ECN_IFNET_ON_REORDER_PERCENT = 71
	, NSTAT_SYSINFO_ECN_IFNET_ON_RXMIT_PERCENT = 72
	, NSTAT_SYSINFO_ECN_IFNET_ON_RXMIT_DROP  = 73
	, NSTAT_SYSINFO_ECN_IFNET_OFF_RTT_AVG    = 74
	, NSTAT_SYSINFO_ECN_IFNET_OFF_RTT_VAR    = 75
	, NSTAT_SYSINFO_ECN_IFNET_OFF_OOPERCENT  = 76
	, NSTAT_SYSINFO_ECN_IFNET_OFF_SACK_EPISODE = 77
	, NSTAT_SYSINFO_ECN_IFNET_OFF_REORDER_PERCENT = 78
	, NSTAT_SYSINFO_ECN_IFNET_OFF_RXMIT_PERCENT = 79
	, NSTAT_SYSINFO_ECN_IFNET_OFF_RXMIT_DROP = 80
	, NSTAT_SYSINFO_ECN_IFNET_ON_TOTAL_TXPKTS = 81
	, NSTAT_SYSINFO_ECN_IFNET_ON_TOTAL_RXMTPKTS = 82
	, NSTAT_SYSINFO_ECN_IFNET_ON_TOTAL_RXPKTS = 83
	, NSTAT_SYSINFO_ECN_IFNET_ON_TOTAL_OOPKTS = 84
	, NSTAT_SYSINFO_ECN_IFNET_ON_DROP_RST = 85
	, NSTAT_SYSINFO_ECN_IFNET_OFF_TOTAL_TXPKTS = 86
	, NSTAT_SYSINFO_ECN_IFNET_OFF_TOTAL_RXMTPKTS = 87
	, NSTAT_SYSINFO_ECN_IFNET_OFF_TOTAL_RXPKTS = 88
	, NSTAT_SYSINFO_ECN_IFNET_OFF_TOTAL_OOPKTS = 89
	, NSTAT_SYSINFO_ECN_IFNET_OFF_DROP_RST = 90
	, NSTAT_SYSINFO_ECN_IFNET_TOTAL_CONN = 91
	, NSTAT_SYSINFO_TFO_COOKIE_WRONG = 92
	, NSTAT_SYSINFO_TFO_NO_COOKIE_RCV = 93
	, NSTAT_SYSINFO_TFO_HEURISTICS_DISABLE = 94
	, NSTAT_SYSINFO_TFO_SEND_BLACKHOLE = 95
	, NSTAT_SYSINFO_KEY_SOCK_MBFLOOR = 96
	, NSTAT_SYSINFO_IFNET_UNSENT_DATA = 97
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_DROPRST = 98
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_DROPRXMT = 99
	, NSTAT_SYSINFO_LIM_IFNET_SIGNATURE = 100
	, NSTAT_SYSINFO_LIM_IFNET_DL_MAX_BANDWIDTH = 101
	, NSTAT_SYSINFO_LIM_IFNET_UL_MAX_BANDWIDTH = 102
	, NSTAT_SYSINFO_LIM_IFNET_PACKET_LOSS_PERCENT = 103
	, NSTAT_SYSINFO_LIM_IFNET_PACKET_OOO_PERCENT = 104
	, NSTAT_SYSINFO_LIM_IFNET_RTT_VARIANCE = 105
	, NSTAT_SYSINFO_LIM_IFNET_RTT_MIN = 106
	, NSTAT_SYSINFO_LIM_IFNET_RTT_AVG = 107
	, NSTAT_SYSINFO_LIM_IFNET_CONN_TIMEOUT_PERCENT = 108
	, NSTAT_SYSINFO_LIM_IFNET_DL_DETECTED = 109
	, NSTAT_SYSINFO_LIM_IFNET_UL_DETECTED = 110
	, NSTAT_SYSINFO_LIM_IFNET_TYPE = 111

	, NSTAT_SYSINFO_API_IF_FLTR_ATTACH = 112
	, NSTAT_SYSINFO_API_IF_FLTR_ATTACH_OS = 113
	, NSTAT_SYSINFO_API_IP_FLTR_ADD = 114
	, NSTAT_SYSINFO_API_IP_FLTR_ADD_OS = 115
	, NSTAT_SYSINFO_API_SOCK_FLTR_ATTACH = 116
	, NSTAT_SYSINFO_API_SOCK_FLTR_ATTACH_OS = 117

	, NSTAT_SYSINFO_API_SOCK_ALLOC_TOTAL = 118
	, NSTAT_SYSINFO_API_SOCK_ALLOC_KERNEL = 119
	, NSTAT_SYSINFO_API_SOCK_ALLOC_KERNEL_OS = 120
	, NSTAT_SYSINFO_API_SOCK_NECP_CLIENTUUID = 121

	, NSTAT_SYSINFO_API_SOCK_DOMAIN_LOCAL = 122
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_ROUTE = 123
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_INET = 124
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_INET6 = 125
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_SYSTEM = 126
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_MULTIPATH = 127
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_KEY = 128
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_NDRV = 129
	, NSTAT_SYSINFO_API_SOCK_DOMAIN_OTHER = 130

	, NSTAT_SYSINFO_API_SOCK_INET_STREAM= 131
	, NSTAT_SYSINFO_API_SOCK_INET_DGRAM = 132
	, NSTAT_SYSINFO_API_SOCK_INET_DGRAM_CONNECTED = 133
	, NSTAT_SYSINFO_API_SOCK_INET_DGRAM_DNS = 134
	, NSTAT_SYSINFO_API_SOCK_INET_DGRAM_NO_DATA = 135

	, NSTAT_SYSINFO_API_SOCK_INET6_STREAM= 136
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM = 137
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM_CONNECTED = 138
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM_DNS = 139
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM_NO_DATA = 140

	, NSTAT_SYSINFO_API_SOCK_INET_MCAST_JOIN = 141
	, NSTAT_SYSINFO_API_SOCK_INET_MCAST_JOIN_OS = 142

	, NSTAT_SYSINFO_API_SOCK_INET6_STREAM_EXTHDR_IN = 143
	, NSTAT_SYSINFO_API_SOCK_INET6_STREAM_EXTHDR_OUT = 144
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM_EXTHDR_IN = 145
	, NSTAT_SYSINFO_API_SOCK_INET6_DGRAM_EXTHDR_OUT = 146

	, NSTAT_SYSINFO_API_NEXUS_FLOW_INET_STREAM = 147
	, NSTAT_SYSINFO_API_NEXUS_FLOW_INET_DATAGRAM = 148

	, NSTAT_SYSINFO_API_NEXUS_FLOW_INET6_STREAM = 149
	, NSTAT_SYSINFO_API_NEXUS_FLOW_INET6_DATAGRAM = 150

	, NSTAT_SYSINFO_API_IFNET_ALLOC = 151
	, NSTAT_SYSINFO_API_IFNET_ALLOC_OS = 152

	, NSTAT_SYSINFO_API_PF_ADDRULE = 153
	, NSTAT_SYSINFO_API_PF_ADDRULE_OS = 154

	, NSTAT_SYSINFO_API_VMNET_START = 155

	, NSTAT_SYSINFO_API_IF_NETAGENT_ENABLED = 156

	, NSTAT_SYSINFO_API_REPORT_INTERVAL = 157

	, NSTAT_SYSINFO_MPTCP_HANDOVER_ATTEMPT = 158
	, NSTAT_SYSINFO_MPTCP_INTERACTIVE_ATTEMPT = 159
	, NSTAT_SYSINFO_MPTCP_AGGREGATE_ATTEMPT = 160
	, NSTAT_SYSINFO_MPTCP_FP_HANDOVER_ATTEMPT = 161                                  
	, NSTAT_SYSINFO_MPTCP_FP_INTERACTIVE_ATTEMPT = 162
	, NSTAT_SYSINFO_MPTCP_FP_AGGREGATE_ATTEMPT = 163
	, NSTAT_SYSINFO_MPTCP_HEURISTIC_FALLBACK = 164
	, NSTAT_SYSINFO_MPTCP_FP_HEURISTIC_FALLBACK = 165
	, NSTAT_SYSINFO_MPTCP_HANDOVER_SUCCESS_WIFI = 166
	, NSTAT_SYSINFO_MPTCP_HANDOVER_SUCCESS_CELL = 167
	, NSTAT_SYSINFO_MPTCP_INTERACTIVE_SUCCESS = 168
	, NSTAT_SYSINFO_MPTCP_AGGREGATE_SUCCESS = 169
	, NSTAT_SYSINFO_MPTCP_FP_HANDOVER_SUCCESS_WIFI = 170
	, NSTAT_SYSINFO_MPTCP_FP_HANDOVER_SUCCESS_CELL = 171
	, NSTAT_SYSINFO_MPTCP_FP_INTERACTIVE_SUCCESS = 172
	, NSTAT_SYSINFO_MPTCP_FP_AGGREGATE_SUCCESS = 173
	, NSTAT_SYSINFO_MPTCP_HANDOVER_CELL_FROM_WIFI = 174
	, NSTAT_SYSINFO_MPTCP_HANDOVER_WIFI_FROM_CELL = 175
	, NSTAT_SYSINFO_MPTCP_INTERACTIVE_CELL_FROM_WIFI = 176
	, NSTAT_SYSINFO_MPTCP_HANDOVER_CELL_BYTES = 177
	, NSTAT_SYSINFO_MPTCP_INTERACTIVE_CELL_BYTES = 178
	, NSTAT_SYSINFO_MPTCP_AGGREGATE_CELL_BYTES = 179
	, NSTAT_SYSINFO_MPTCP_HANDOVER_ALL_BYTES = 180
	, NSTAT_SYSINFO_MPTCP_INTERACTIVE_ALL_BYTES = 181
	, NSTAT_SYSINFO_MPTCP_AGGREGATE_ALL_BYTES = 182
	, NSTAT_SYSINFO_MPTCP_BACK_TO_WIFI = 183
	, NSTAT_SYSINFO_MPTCP_WIFI_PROXY = 184
	, NSTAT_SYSINFO_MPTCP_CELL_PROXY = 185
	, NSTAT_SYSINFO_ECN_IFNET_FALLBACK_SYNRST = 186
	, NSTAT_SYSINFO_MPTCP_TRIGGERED_CELL = 187

                                                                       
#define NSTAT_SYSINFO_ENUM_VERSION      20180416
};

#define NSTAT_SYSINFO_API_FIRST NSTAT_SYSINFO_API_IF_FLTR_ATTACH
#define NSTAT_SYSINFO_API_LAST  NSTAT_SYSINFO_API_REPORT_INTERVAL

#pragma mark -- Network Statistics Providers --


                       

#define NSTAT_IFNET_IS_UNKNOWN_TYPE      0x0001
#define NSTAT_IFNET_IS_LOOPBACK          0x0002
#define NSTAT_IFNET_IS_CELLULAR          0x0004
#define NSTAT_IFNET_IS_WIFI              0x0008
#define NSTAT_IFNET_IS_WIRED             0x0010
#define NSTAT_IFNET_IS_AWDL              0x0020
#define NSTAT_IFNET_IS_EXPENSIVE         0x0040
#define NSTAT_IFNET_IS_VPN               0x0080
#define NSTAT_IFNET_VIA_CELLFALLBACK     0x0100
#define NSTAT_IFNET_IS_COMPANIONLINK     0x0200
#define NSTAT_IFNET_IS_CONSTRAINED       0x0400
                                                                      
                                                                                          
#define NSTAT_IFNET_IS_LOCAL             0x0800
#define NSTAT_IFNET_IS_NON_LOCAL         0x1000
                                                                 
#define NSTAT_IFNET_ROUTE_VALUE_UNOBTAINABLE      0x2000
#define NSTAT_IFNET_FLOWSWITCH_VALUE_UNOBTAINABLE 0x4000


typedef enum {
	NSTAT_PROVIDER_NONE           = 0
	, NSTAT_PROVIDER_ROUTE        = 1
	, NSTAT_PROVIDER_TCP_KERNEL   = 2
	, NSTAT_PROVIDER_TCP_USERLAND = 3
	, NSTAT_PROVIDER_UDP_KERNEL   = 4
	, NSTAT_PROVIDER_UDP_USERLAND = 5
	, NSTAT_PROVIDER_IFNET        = 6
	, NSTAT_PROVIDER_SYSINFO      = 7
	, NSTAT_PROVIDER_QUIC_USERLAND = 8
} nstat_provider_type_t;
#define NSTAT_PROVIDER_LAST NSTAT_PROVIDER_QUIC_USERLAND
#define NSTAT_PROVIDER_COUNT (NSTAT_PROVIDER_LAST+1)

typedef struct nstat_route_add_param {
	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} dst;
	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} mask;
	u_int32_t       ifindex;
} nstat_route_add_param;

typedef struct nstat_tcp_add_param {
	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} local;
	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} remote;
} nstat_tcp_add_param;

typedef struct nstat_tcp_descriptor {
	u_int64_t       upid __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       eupid __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       start_timestamp __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       timestamp __attribute__((aligned(sizeof(u_int64_t))));

	u_int64_t       rx_transfer_size __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       tx_transfer_size __attribute__((aligned(sizeof(u_int64_t))));

	activity_bitmap_t activity_bitmap;

	u_int32_t       ifindex;
	u_int32_t       state;

	u_int32_t       sndbufsize;
	u_int32_t       sndbufused;
	u_int32_t       rcvbufsize;
	u_int32_t       rcvbufused;
	u_int32_t       txunacked;
	u_int32_t       txwindow;
	u_int32_t       txcwindow;
	u_int32_t       traffic_class;
	u_int32_t       traffic_mgt_flags;

	u_int32_t       pid;
	u_int32_t       epid;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} local;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} remote;

	char            cc_algo[16];
	char            pname[64];

	uuid_t          uuid;
	uuid_t          euuid;
	uuid_t          vuuid;
	uuid_t          fuuid;
	union {
		struct tcp_conn_status connstatus;
		                                                    
		uint8_t                                 __pad_connstatus[4];
	};
	uint16_t        ifnet_properties        __attribute__((aligned(4)));

	u_int8_t        reserved[6];
} nstat_tcp_descriptor;

typedef struct nstat_tcp_add_param      nstat_udp_add_param;

typedef struct nstat_udp_descriptor {
	u_int64_t       upid __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       eupid __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       start_timestamp __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       timestamp __attribute__((aligned(sizeof(u_int64_t))));

	activity_bitmap_t activity_bitmap;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} local;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
	} remote;

	u_int32_t       ifindex;

	u_int32_t       rcvbufsize;
	u_int32_t       rcvbufused;
	u_int32_t       traffic_class;

	u_int32_t       pid;
	char            pname[64];
	u_int32_t       epid;

	uuid_t          uuid;
	uuid_t          euuid;
	uuid_t          vuuid;
	uuid_t          fuuid;
	uint16_t        ifnet_properties;

	u_int8_t        reserved[6];
} nstat_udp_descriptor;

  
                                                                             
                                            
                                                                       
                     
   
typedef struct nstat_tcp_add_param      nstat_quic_add_param;
typedef struct nstat_tcp_descriptor     nstat_quic_descriptor;

typedef struct nstat_route_descriptor {
	u_int64_t       id __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       parent_id __attribute__((aligned(sizeof(u_int64_t))));
	u_int64_t       gateway_id __attribute__((aligned(sizeof(u_int64_t))));

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
		struct sockaddr         sa;
	} dst;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
		struct sockaddr         sa;
	} mask;

	union{
		struct sockaddr_in      v4;
		struct sockaddr_in6     v6;
		struct sockaddr         sa;
	} gateway;

	u_int32_t       ifindex;
	u_int32_t       flags;

	u_int8_t        reserved[4];
} nstat_route_descriptor;

typedef struct nstat_ifnet_add_param {
	u_int64_t       threshold __attribute__((aligned(sizeof(u_int64_t))));
	u_int32_t       ifindex;

	u_int8_t        reserved[4];
} nstat_ifnet_add_param;

typedef struct nstat_ifnet_desc_cellular_status {
	u_int32_t valid_bitmask;                                       
#define NSTAT_IFNET_DESC_CELL_LINK_QUALITY_METRIC_VALID         0x1
#define NSTAT_IFNET_DESC_CELL_UL_EFFECTIVE_BANDWIDTH_VALID      0x2
#define NSTAT_IFNET_DESC_CELL_UL_MAX_BANDWIDTH_VALID            0x4
#define NSTAT_IFNET_DESC_CELL_UL_MIN_LATENCY_VALID              0x8
#define NSTAT_IFNET_DESC_CELL_UL_EFFECTIVE_LATENCY_VALID        0x10
#define NSTAT_IFNET_DESC_CELL_UL_MAX_LATENCY_VALID              0x20
#define NSTAT_IFNET_DESC_CELL_UL_RETXT_LEVEL_VALID              0x40
#define NSTAT_IFNET_DESC_CELL_UL_BYTES_LOST_VALID               0x80
#define NSTAT_IFNET_DESC_CELL_UL_MIN_QUEUE_SIZE_VALID           0x100
#define NSTAT_IFNET_DESC_CELL_UL_AVG_QUEUE_SIZE_VALID           0x200
#define NSTAT_IFNET_DESC_CELL_UL_MAX_QUEUE_SIZE_VALID           0x400
#define NSTAT_IFNET_DESC_CELL_DL_EFFECTIVE_BANDWIDTH_VALID      0x800
#define NSTAT_IFNET_DESC_CELL_DL_MAX_BANDWIDTH_VALID            0x1000
#define NSTAT_IFNET_DESC_CELL_CONFIG_INACTIVITY_TIME_VALID      0x2000
#define NSTAT_IFNET_DESC_CELL_CONFIG_BACKOFF_TIME_VALID         0x4000
#define NSTAT_IFNET_DESC_CELL_MSS_RECOMMENDED_VALID             0x8000
	u_int32_t link_quality_metric;
	u_int32_t ul_effective_bandwidth;                                      
                                                                
	u_int32_t ul_max_bandwidth;                                      
                                         
	u_int32_t ul_min_latency;                                             
                                      
	u_int32_t ul_effective_latency;                                       
                                                      
	u_int32_t ul_max_latency;                                         
                                      
	u_int32_t ul_retxt_level;                            
#define NSTAT_IFNET_DESC_CELL_UL_RETXT_LEVEL_NONE       1
#define NSTAT_IFNET_DESC_CELL_UL_RETXT_LEVEL_LOW        2
#define NSTAT_IFNET_DESC_CELL_UL_RETXT_LEVEL_MEDIUM     3
#define NSTAT_IFNET_DESC_CELL_UL_RETXT_LEVEL_HIGH       4

	u_int32_t ul_bytes_lost;                                          
                                       
	u_int32_t ul_min_queue_size;                             
	u_int32_t ul_avg_queue_size;                             
	u_int32_t ul_max_queue_size;                             
	u_int32_t dl_effective_bandwidth;                                     
                                                                   
	u_int32_t dl_max_bandwidth;                                        
                                         
	u_int32_t config_inactivity_time;         
	u_int32_t config_backoff_time;                                         
#define NSTAT_IFNET_DESC_MSS_RECOMMENDED_NONE   0x0
#define NSTAT_IFNET_DESC_MSS_RECOMMENDED_MEDIUM 0x1
#define NSTAT_IFNET_DESC_MSS_RECOMMENDED_LOW    0x2
	u_int16_t mss_recommended;                      
	u_int8_t        reserved[2];
} nstat_ifnet_desc_cellular_status;

typedef struct nstat_ifnet_desc_wifi_status {
	u_int32_t valid_bitmask;
#define NSTAT_IFNET_DESC_WIFI_LINK_QUALITY_METRIC_VALID         0x1
#define NSTAT_IFNET_DESC_WIFI_UL_EFFECTIVE_BANDWIDTH_VALID      0x2
#define NSTAT_IFNET_DESC_WIFI_UL_MAX_BANDWIDTH_VALID            0x4
#define NSTAT_IFNET_DESC_WIFI_UL_MIN_LATENCY_VALID              0x8
#define NSTAT_IFNET_DESC_WIFI_UL_EFFECTIVE_LATENCY_VALID        0x10
#define NSTAT_IFNET_DESC_WIFI_UL_MAX_LATENCY_VALID              0x20
#define NSTAT_IFNET_DESC_WIFI_UL_RETXT_LEVEL_VALID              0x40
#define NSTAT_IFNET_DESC_WIFI_UL_ERROR_RATE_VALID               0x80
#define NSTAT_IFNET_DESC_WIFI_UL_BYTES_LOST_VALID               0x100
#define NSTAT_IFNET_DESC_WIFI_DL_EFFECTIVE_BANDWIDTH_VALID      0x200
#define NSTAT_IFNET_DESC_WIFI_DL_MAX_BANDWIDTH_VALID            0x400
#define NSTAT_IFNET_DESC_WIFI_DL_MIN_LATENCY_VALID              0x800
#define NSTAT_IFNET_DESC_WIFI_DL_EFFECTIVE_LATENCY_VALID        0x1000
#define NSTAT_IFNET_DESC_WIFI_DL_MAX_LATENCY_VALID              0x2000
#define NSTAT_IFNET_DESC_WIFI_DL_ERROR_RATE_VALID               0x4000
#define NSTAT_IFNET_DESC_WIFI_CONFIG_FREQUENCY_VALID            0x8000
#define NSTAT_IFNET_DESC_WIFI_CONFIG_MULTICAST_RATE_VALID       0x10000
#define NSTAT_IFNET_DESC_WIFI_CONFIG_SCAN_COUNT_VALID           0x20000
#define NSTAT_IFNET_DESC_WIFI_CONFIG_SCAN_DURATION_VALID        0x40000
	u_int32_t link_quality_metric;                          
	u_int32_t ul_effective_bandwidth;                                      
                                                                
	u_int32_t ul_max_bandwidth;                                      
                                         
	u_int32_t ul_min_latency;                                             
                                      
	u_int32_t ul_effective_latency;                                       
                                                      
	u_int32_t ul_max_latency;                                             
                                      
	u_int32_t ul_retxt_level;                            
#define NSTAT_IFNET_DESC_WIFI_UL_RETXT_LEVEL_NONE       1
#define NSTAT_IFNET_DESC_WIFI_UL_RETXT_LEVEL_LOW        2
#define NSTAT_IFNET_DESC_WIFI_UL_RETXT_LEVEL_MEDIUM     3
#define NSTAT_IFNET_DESC_WIFI_UL_RETXT_LEVEL_HIGH       4

	u_int32_t ul_bytes_lost;                                          
                                       
	u_int32_t ul_error_rate;                                           
                                                              
	u_int32_t dl_effective_bandwidth;                                     
                                                                   
	u_int32_t dl_max_bandwidth;                                        
                                         
	  
                                                                     
                                                                      
                                                                     
                                                    
    
	u_int32_t dl_min_latency;                                               
	u_int32_t dl_effective_latency;                                      
                                                 
	u_int32_t dl_max_latency;                                               
	u_int32_t dl_error_rate;                                             
	u_int32_t config_frequency;                   
#define NSTAT_IFNET_DESC_WIFI_CONFIG_FREQUENCY_2_4_GHZ  1
#define NSTAT_IFNET_DESC_WIFI_CONFIG_FREQUENCY_5_0_GHZ  2
	u_int32_t config_multicast_rate;          
	u_int32_t scan_count;                                            
	u_int32_t scan_duration;                          
} nstat_ifnet_desc_wifi_status;

enum{
	NSTAT_IFNET_DESC_LINK_STATUS_TYPE_NONE = 0
	, NSTAT_IFNET_DESC_LINK_STATUS_TYPE_CELLULAR = 1
	, NSTAT_IFNET_DESC_LINK_STATUS_TYPE_WIFI = 2
	, NSTAT_IFNET_DESC_LINK_STATUS_TYPE_ETHERNET = 3
};

typedef struct nstat_ifnet_desc_link_status {
	u_int32_t       link_status_type;
	union {
		nstat_ifnet_desc_cellular_status        cellular;
		nstat_ifnet_desc_wifi_status            wifi;
	} u;
} nstat_ifnet_desc_link_status;

#ifndef IF_DESCSIZE
#define IF_DESCSIZE 128
#endif
typedef struct nstat_ifnet_descriptor {
	u_int64_t                       threshold __attribute__((aligned(sizeof(u_int64_t))));
	u_int32_t                       ifindex;
	nstat_ifnet_desc_link_status    link_status;
	unsigned int            type;
	char                            description[IF_DESCSIZE];
	char                            name[IFNAMSIZ + 1];
	u_int8_t                        reserved[3];
} nstat_ifnet_descriptor;

typedef struct nstat_sysinfo_descriptor {
	u_int32_t       flags;
} nstat_sysinfo_descriptor;

typedef struct nstat_sysinfo_add_param {
	                                                                    
	u_int32_t       flags;
} nstat_sysinfo_add_param;

#define NSTAT_SYSINFO_MBUF_STATS        0x0001
#define NSTAT_SYSINFO_TCP_STATS         0x0002
#define NSTAT_SYSINFO_IFNET_ECN_STATS   0x0003
#define NSTAT_SYSINFO_LIM_STATS         0x0004                               
#define NSTAT_SYSINFO_NET_API_STATS     0x0005                         

#pragma mark -- Network Statistics User Client --

#define NET_STAT_CONTROL_NAME   "com.apple.network.statistics"

enum{
	                            
	NSTAT_MSG_TYPE_SUCCESS               = 0
	, NSTAT_MSG_TYPE_ERROR               = 1

	               
	, NSTAT_MSG_TYPE_ADD_SRC             = 1001
	, NSTAT_MSG_TYPE_ADD_ALL_SRCS        = 1002
	, NSTAT_MSG_TYPE_REM_SRC             = 1003
	, NSTAT_MSG_TYPE_QUERY_SRC           = 1004
	, NSTAT_MSG_TYPE_GET_SRC_DESC        = 1005
	, NSTAT_MSG_TYPE_SET_FILTER          = 1006            
	, NSTAT_MSG_TYPE_GET_UPDATE          = 1007
	, NSTAT_MSG_TYPE_SUBSCRIBE_SYSINFO   = 1008

	                             
	, NSTAT_MSG_TYPE_SRC_ADDED           = 10001
	, NSTAT_MSG_TYPE_SRC_REMOVED         = 10002
	, NSTAT_MSG_TYPE_SRC_DESC            = 10003
	, NSTAT_MSG_TYPE_SRC_COUNTS          = 10004
	, NSTAT_MSG_TYPE_SYSINFO_COUNTS      = 10005
	, NSTAT_MSG_TYPE_SRC_UPDATE          = 10006
};

enum{
	NSTAT_SRC_REF_ALL       = 0xffffffffffffffffULL
	, NSTAT_SRC_REF_INVALID  = 0
};

                          
enum{
	NSTAT_FILTER_NOZEROBYTES             = 0x00000001
};

                            
enum{
	NSTAT_FILTER_ACCEPT_UNKNOWN           = 0x00000001
	, NSTAT_FILTER_ACCEPT_LOOPBACK        = 0x00000002
	, NSTAT_FILTER_ACCEPT_CELLULAR        = 0x00000004
	, NSTAT_FILTER_ACCEPT_WIFI            = 0x00000008
	, NSTAT_FILTER_ACCEPT_WIRED           = 0x00000010
	, NSTAT_FILTER_ACCEPT_AWDL            = 0x00000020
	, NSTAT_FILTER_ACCEPT_EXPENSIVE       = 0x00000040
	, NSTAT_FILTER_ACCEPT_CELLFALLBACK    = 0x00000100
	, NSTAT_FILTER_ACCEPT_COMPANIONLINK   = 0x00000200
	, NSTAT_FILTER_ACCEPT_IS_CONSTRAINED  = 0x00000400
	, NSTAT_FILTER_ACCEPT_IS_LOCAL        = 0x00000800
	, NSTAT_FILTER_ACCEPT_IS_NON_LOCAL    = 0x00001000
	, NSTAT_FILTER_ACCEPT_ROUTE_VAL_ERR   = 0x00002000
	, NSTAT_FILTER_ACCEPT_FLOWSWITCH_ERR  = 0x00004000
	, NSTAT_FILTER_IFNET_FLAGS            = 0x0000FFFF

	, NSTAT_FILTER_UDP_INTERFACE_ATTACH   = 0x00010000
	, NSTAT_FILTER_UDP_FLAGS              = 0x00010000

	, NSTAT_FILTER_TCP_INTERFACE_ATTACH   = 0x00040000
	, NSTAT_FILTER_TCP_NO_EARLY_CLOSE     = 0x00080000
	, NSTAT_FILTER_TCP_FLAGS              = 0x000C0000

	, NSTAT_FILTER_SUPPRESS_SRC_ADDED     = 0x00100000
	, NSTAT_FILTER_REQUIRE_SRC_ADDED      = 0x00200000
	, NSTAT_FILTER_PROVIDER_NOZEROBYTES   = 0x00400000

	, NSTAT_FILTER_SPECIFIC_USER_BY_PID   = 0x01000000
	, NSTAT_FILTER_SPECIFIC_USER_BY_EPID  = 0x02000000
	, NSTAT_FILTER_SPECIFIC_USER_BY_UUID  = 0x04000000
	, NSTAT_FILTER_SPECIFIC_USER_BY_EUUID = 0x08000000
	, NSTAT_FILTER_SPECIFIC_USER          = 0x0F000000
};

enum{
	NSTAT_MSG_HDR_FLAG_SUPPORTS_AGGREGATE   = 1 << 0,
	NSTAT_MSG_HDR_FLAG_CONTINUATION         = 1 << 1,
	NSTAT_MSG_HDR_FLAG_CLOSING              = 1 << 2,
};

typedef struct nstat_msg_hdr {
	u_int64_t       context __attribute__((aligned(sizeof(u_int64_t))));
	u_int32_t       type;
	u_int16_t       length;
	u_int16_t       flags;
} nstat_msg_hdr;

#define MAX_NSTAT_MSG_HDR_LENGTH    65532

typedef struct nstat_msg_error {
	nstat_msg_hdr   hdr;
	u_int32_t               error;                
	u_int8_t                reserved[4];
} nstat_msg_error;

#define NSTAT_ADD_SRC_FIELDS            \
	nstat_msg_hdr		hdr;            \
	nstat_provider_id_t	provider;       \
	u_int8_t			reserved[4]     \

typedef struct nstat_msg_add_src {
	NSTAT_ADD_SRC_FIELDS;
	u_int8_t        param[];
} nstat_msg_add_src_req;

typedef struct nstat_msg_add_src_header {
	NSTAT_ADD_SRC_FIELDS;
} nstat_msg_add_src_header;

typedef struct nstat_msg_add_src_convenient {
	nstat_msg_add_src_header        hdr;
	union {
		nstat_route_add_param   route;
		nstat_tcp_add_param             tcp;
		nstat_udp_add_param             udp;
		nstat_ifnet_add_param   ifnet;
		nstat_sysinfo_add_param sysinfo;
	};
} nstat_msg_add_src_convenient;

#undef NSTAT_ADD_SRC_FIELDS

typedef struct nstat_msg_add_all_srcs {
	nstat_msg_hdr           hdr;
	u_int64_t               filter __attribute__((aligned(sizeof(u_int64_t))));
	nstat_event_flags_t     events __attribute__((aligned(sizeof(u_int64_t))));
	nstat_provider_id_t     provider;
	pid_t                   target_pid;
	uuid_t                  target_uuid;
} nstat_msg_add_all_srcs;

typedef struct nstat_msg_src_added {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
	nstat_provider_id_t     provider;
	u_int8_t                reserved[4];
} nstat_msg_src_added;

typedef struct nstat_msg_rem_src {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
} nstat_msg_rem_src_req;

typedef struct nstat_msg_get_src_description {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
} nstat_msg_get_src_description;

typedef struct nstat_msg_set_filter {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
	u_int32_t               filter;
	u_int8_t                reserved[4];
} nstat_msg_set_filter;

#define NSTAT_SRC_DESCRIPTION_FIELDS                                                                                            \
	nstat_msg_hdr		hdr;                                                                                                            \
	nstat_src_ref_t		srcref __attribute__((aligned(sizeof(u_int64_t))));                     \
	nstat_event_flags_t	event_flags __attribute__((aligned(sizeof(u_int64_t))));        \
	nstat_provider_id_t	provider;                                                                                                       \
	u_int8_t			reserved[4]

typedef struct nstat_msg_src_description {
	NSTAT_SRC_DESCRIPTION_FIELDS;
	u_int8_t        data[];
} nstat_msg_src_description;

typedef struct nstat_msg_src_description_header {
	NSTAT_SRC_DESCRIPTION_FIELDS;
} nstat_msg_src_description_header;

typedef struct nstat_msg_src_description_convenient {
	nstat_msg_src_description_header    hdr;
	union {
		nstat_tcp_descriptor            tcp;
		nstat_udp_descriptor            udp;
		nstat_route_descriptor          route;
		nstat_ifnet_descriptor          ifnet;
		nstat_sysinfo_descriptor        sysinfo;
		nstat_quic_descriptor           quic;
	};
} nstat_msg_src_description_convenient;

#undef NSTAT_SRC_DESCRIPTION_FIELDS

typedef struct nstat_msg_query_src {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
} nstat_msg_query_src_req;

typedef struct nstat_msg_src_counts {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
	nstat_event_flags_t     event_flags __attribute__((aligned(sizeof(u_int64_t))));
	nstat_counts            counts;
} nstat_msg_src_counts;

#define NSTAT_SRC_UPDATE_FIELDS                                                                                                         \
	nstat_msg_hdr		hdr;                                                                                                            \
	nstat_src_ref_t		srcref __attribute__((aligned(sizeof(u_int64_t))));                     \
	nstat_event_flags_t	event_flags __attribute__((aligned(sizeof(u_int64_t))));        \
	nstat_counts		counts;                                                                                                         \
	nstat_provider_id_t	provider;                                                                                                       \
	u_int8_t			reserved[4]

typedef struct nstat_msg_src_update {
	NSTAT_SRC_UPDATE_FIELDS;
	u_int8_t        data[];
} nstat_msg_src_update;

typedef struct nstat_msg_src_update_hdr {
	NSTAT_SRC_UPDATE_FIELDS;
} nstat_msg_src_update_hdr;

typedef struct nstat_msg_src_update_convenient {
	nstat_msg_src_update_hdr                hdr;
	union {
		nstat_tcp_descriptor            tcp;
		nstat_udp_descriptor            udp;
		nstat_route_descriptor          route;
		nstat_ifnet_descriptor          ifnet;
		nstat_sysinfo_descriptor        sysinfo;
		nstat_quic_descriptor           quic;
	};
} nstat_msg_src_update_convenient;

#undef NSTAT_SRC_UPDATE_FIELDS

typedef struct nstat_msg_src_removed {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
} nstat_msg_src_removed;

typedef struct nstat_msg_sysinfo_counts {
	nstat_msg_hdr           hdr;
	nstat_src_ref_t         srcref __attribute__((aligned(sizeof(u_int64_t))));
	nstat_sysinfo_counts    counts;
}  nstat_msg_sysinfo_counts;

#pragma mark -- Statitiscs about Network Statistics --

struct nstat_stats {
	u_int32_t nstat_successmsgfailures;
	u_int32_t nstat_sendcountfailures;
	u_int32_t nstat_sysinfofailures;
	u_int32_t nstat_srcupatefailures;
	u_int32_t nstat_descriptionfailures;
	u_int32_t nstat_msgremovedfailures;
	u_int32_t nstat_srcaddedfailures;
	u_int32_t nstat_msgerrorfailures;
	u_int32_t nstat_copy_descriptor_failures;
	u_int32_t nstat_provider_counts_failures;
	u_int32_t nstat_control_send_description_failures;
	u_int32_t nstat_control_send_goodbye_failures;
	u_int32_t nstat_flush_accumulated_msgs_failures;
	u_int32_t nstat_accumulate_msg_failures;
	u_int32_t nstat_control_cleanup_source_failures;
	u_int32_t nstat_handle_msg_failures;
};

#endif              

#ifdef XNU_KERNEL_PRIVATE
#include <sys/mcache.h>

#pragma mark -- System Information Internal Support --

typedef struct nstat_sysinfo_mbuf_stats {
	u_int32_t       total_256b;                                
	u_int32_t       total_2kb;                                
	u_int32_t       total_4kb;                                
	u_int32_t       total_16kb;                                
	u_int32_t       sbmb_total;                                          
	u_int32_t       sb_atmbuflimit;                                                         
	u_int32_t       draincnt;                                                                             
	u_int32_t       memreleased;                                                      
	u_int32_t       sbmb_floor;                                           
} nstat_sysinfo_mbuf_stats;

typedef struct nstat_sysinfo_tcp_stats {
	                                                                          
	u_int32_t       ipv4_avgrtt;                              
	u_int32_t       ipv6_avgrtt;                              
	u_int32_t       send_plr;                                            
	u_int32_t       recv_plr;                                              
	u_int32_t       send_tlrto_rate;                                          
	u_int32_t       send_reorder_rate;                                     
	u_int32_t       connection_attempts;                                     
	u_int32_t       connection_accepts;                                    
	u_int32_t       ecn_client_enabled;                                         
	u_int32_t       ecn_server_enabled;                                         
	u_int32_t       ecn_client_setup;                                                       
	u_int32_t       ecn_server_setup;                                                       
	u_int32_t       ecn_client_success;                                                                       
	u_int32_t       ecn_server_success;                                                                       
	u_int32_t       ecn_not_supported;                                                          
	u_int32_t       ecn_lost_syn;                                          
	u_int32_t       ecn_lost_synack;                                            
	u_int32_t       ecn_recv_ce;                                             
	u_int32_t       ecn_recv_ece;                                              
	u_int32_t       ecn_sent_ece;                                              
	u_int32_t       ecn_conn_recv_ce;                                                                
	u_int32_t       ecn_conn_recv_ece;                                                                 
	u_int32_t       ecn_conn_plnoce;                                                                             
	u_int32_t       ecn_conn_pl_ce;                                                              
	u_int32_t       ecn_conn_nopl_ce;                                                                          
	u_int32_t       ecn_fallback_synloss;                                                       
	u_int32_t       ecn_fallback_reorder;                                                                     
	u_int32_t       ecn_fallback_ce;                                                                   
	u_int32_t       tfo_syn_data_rcv;                                                          
	u_int32_t       tfo_cookie_req_rcv;                                            
	u_int32_t       tfo_cookie_sent;                                                         
	u_int32_t       tfo_cookie_invalid;                                            
	u_int32_t       tfo_cookie_req;                                                 
	u_int32_t       tfo_cookie_rcv;                                              
	u_int32_t       tfo_syn_data_sent;                                           
	u_int32_t       tfo_syn_data_acked;                                                        
	u_int32_t       tfo_syn_loss;                                                              
	u_int32_t       tfo_blackhole;                                                             
	u_int32_t       tfo_cookie_wrong;                                         
	u_int32_t       tfo_no_cookie_rcv;                                                    
	u_int32_t       tfo_heuristics_disable;                                         
	u_int32_t       tfo_sndblackhole;                                                        
	u_int32_t       mptcp_handover_attempt;                                                         
	u_int32_t       mptcp_interactive_attempt;                                                                 
	u_int32_t       mptcp_aggregate_attempt;                                                                 
	u_int32_t       mptcp_fp_handover_attempt;                                                           
	u_int32_t       mptcp_fp_interactive_attempt;
	u_int32_t       mptcp_fp_aggregate_attempt;
	u_int32_t       mptcp_heuristic_fallback;                                                                               
	u_int32_t       mptcp_fp_heuristic_fallback;                                                   
	u_int32_t       mptcp_handover_success_wifi;                                                                                      
	u_int32_t       mptcp_handover_success_cell;                                                                                      
	u_int32_t       mptcp_interactive_success;                                                                                      
	u_int32_t       mptcp_aggregate_success;                                                        
	u_int32_t       mptcp_fp_handover_success_wifi;                                                      
	u_int32_t       mptcp_fp_handover_success_cell;
	u_int32_t       mptcp_fp_interactive_success;
	u_int32_t       mptcp_fp_aggregate_success;
	u_int32_t       mptcp_handover_cell_from_wifi;                                                                                     
	u_int32_t       mptcp_handover_wifi_from_cell;                                                                                     
	u_int32_t       mptcp_interactive_cell_from_wifi;                                                                                             
	u_int32_t       mptcp_back_to_wifi;                                                                                                          
	u_int64_t       mptcp_handover_cell_bytes;                                                                                                               
	u_int64_t       mptcp_interactive_cell_bytes;                                             
	u_int64_t       mptcp_aggregate_cell_bytes;
	u_int64_t       mptcp_handover_all_bytes;                                                           
	u_int64_t       mptcp_interactive_all_bytes;
	u_int64_t       mptcp_aggregate_all_bytes;
	u_int32_t       mptcp_wifi_proxy;                                                                                       
	u_int32_t       mptcp_cell_proxy;                                                                                       
	u_int32_t       mptcp_triggered_cell;                                                                                 
	u_int32_t       _padding;
	                                                                          
} nstat_sysinfo_tcp_stats;
#define NSTAT_SYSINFO_TCP_STATS_COUNT   71

enum {
	NSTAT_IFNET_ECN_PROTO_IPV4 = 1
	, NSTAT_IFNET_ECN_PROTO_IPV6
};

enum {
	NSTAT_IFNET_ECN_TYPE_CELLULAR = 1
	, NSTAT_IFNET_ECN_TYPE_WIFI
	, NSTAT_IFNET_ECN_TYPE_ETHERNET
};

typedef struct nstat_sysinfo_ifnet_ecn_stats {
	u_int32_t               ifnet_proto;
	u_int32_t               ifnet_type;
	struct if_tcp_ecn_stat  ecn_stat;
} nstat_sysinfo_ifnet_ecn_stats;

                                                              
#define NSTAT_LIM_STAT_KEYVAL_COUNT     12
typedef struct nstat_sysinfo_lim_stats {
	u_int8_t                ifnet_signature[NSTAT_SYSINFO_KEYVAL_STRING_MAXSIZE];
	u_int32_t               ifnet_siglen;
	u_int32_t               ifnet_type;
	struct if_lim_perf_stat lim_stat;
} nstat_sysinfo_lim_stats;

#define NSTAT_NET_API_STAT_KEYVAL_COUNT (NSTAT_SYSINFO_API_LAST - NSTAT_SYSINFO_API_FIRST + 1)
typedef struct nstat_sysinfo_net_api_stats {
	u_int32_t               report_interval;
	u_int32_t               _padding;
	struct net_api_stats    net_api_stats;
} nstat_sysinfo_net_api_stats;

typedef struct nstat_sysinfo_data {
	uint32_t                flags;
	uint32_t                unsent_data_cnt;                      
	union {
		nstat_sysinfo_mbuf_stats mb_stats;
		nstat_sysinfo_tcp_stats tcp_stats;
		nstat_sysinfo_ifnet_ecn_stats ifnet_ecn_stats;
		nstat_sysinfo_lim_stats lim_stats;
		nstat_sysinfo_net_api_stats net_api_stats;
	} u;
} nstat_sysinfo_data;

#pragma mark -- Generic Network Statistics Provider --

typedef void *  nstat_provider_cookie_t;

#pragma mark -- Route Statistics Gathering Functions --
struct rtentry;

enum{
	NSTAT_TX_FLAG_RETRANSMIT        = 1
};

enum{
	NSTAT_RX_FLAG_DUPLICATE         = 1,
	NSTAT_RX_FLAG_OUT_OF_ORDER      = 2
};

                                                               
extern int      nstat_collect;

void nstat_init(void);

                            
void nstat_route_connect_attempt(struct rtentry *rte);
void nstat_route_connect_success(struct rtentry *rte);
void nstat_route_tx(struct rtentry *rte, u_int32_t packets, u_int32_t bytes, u_int32_t flags);
void nstat_route_rx(struct rtentry *rte, u_int32_t packets, u_int32_t bytes, u_int32_t flags);
void nstat_route_rtt(struct rtentry *rte, u_int32_t rtt, u_int32_t rtt_var);
void nstat_route_update(struct rtentry *rte, uint32_t connect_attempts, uint32_t connect_successes,
    uint32_t rx_packets, uint32_t rx_bytes, uint32_t rx_duplicatebytes, uint32_t rx_outoforderbytes,
    uint32_t tx_packets, uint32_t tx_bytes, uint32_t tx_retransmit,
    uint32_t rtt, uint32_t rtt_var);
struct nstat_counts* nstat_route_attach(struct rtentry  *rte);
void nstat_route_detach(struct rtentry *rte);

                  
struct inpcb;
void nstat_tcp_new_pcb(struct inpcb *inp);
void nstat_udp_new_pcb(struct inpcb *inp);
void nstat_route_new_entry(struct rtentry *rt);
void nstat_pcb_detach(struct inpcb *inp);
void nstat_pcb_event(struct inpcb *inp, u_int64_t event);
void nstat_pcb_cache(struct inpcb *inp);
void nstat_pcb_invalidate_cache(struct inpcb *inp);


void nstat_ifnet_threshold_reached(unsigned int ifindex);

void nstat_sysinfo_send_data(struct nstat_sysinfo_data *);

int ntstat_tcp_progress_indicators(struct sysctl_req *req);


                                         
u_int16_t nstat_ifnet_to_flags(struct ifnet *ifp);

                                                             
                                                                   
                                                                  
                                  
#if defined(__LP64__)
#define locked_add_64(__addr, __count) do { \
	*(__addr) += (__count); \
} while (0)
#else
#define locked_add_64(__addr, __count) do { \
	atomic_add_64((__addr), (__count)); \
} while (0)
#endif

#endif                         

#endif                   
