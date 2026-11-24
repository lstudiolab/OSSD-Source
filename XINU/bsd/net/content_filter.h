/*
 * Copyright (c) 2013-2019 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef __CONTENT_FILTER_H__
#define __CONTENT_FILTER_H__

#include <sys/param.h>
#include <sys/types.h>
#include <sys/_types/_timeval64.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <stdint.h>
#include <corecrypto/ccsha2.h>

#ifdef BSD_KERNEL_PRIVATE
#include <sys/mbuf.h>
#include <sys/socketvar.h>
#endif                         

__BEGIN_DECLS

#ifdef PRIVATE

  
                                                          
                                                                  
                                                              
                                 
   
#define CONTENT_FILTER_CONTROL_NAME "com.apple.content-filter"

  
                           
   
typedef uint64_t cfil_sock_id_t;

#define CFIL_SOCK_ID_NONE UINT64_MAX


  
                             
                                                                           
                                       
   
#define CFIL_OPT_NECP_CONTROL_UNIT      1                     

  
                           
                                                                  
   
#define CFIL_OPT_GET_SOCKET_INFO        2                     

  
                            
  
                                                              
   
struct cfil_opt_sock_info {
	cfil_sock_id_t  cfs_sock_id;
	int                             cfs_sock_family;                          
	int                             cfs_sock_type;                                
	int                             cfs_sock_protocol;                            
	union sockaddr_in_4_6   cfs_local;
	union sockaddr_in_4_6   cfs_remote;
	pid_t                   cfs_pid;
	pid_t                   cfs_e_pid;
	uuid_t                  cfs_uuid;
	uuid_t                  cfs_e_uuid;
};

  
                                               
   
#if !TARGET_OS_OSX && !defined(XNU_TARGET_OS_OSX)
#define CFIL_MAX_FILTER_COUNT   2
#else
#define CFIL_MAX_FILTER_COUNT   8
#endif


  
                 
   
#define CFIL_CRYPTO 1
#define CFIL_CRYPTO_SIGNATURE_SIZE 32
#define CFIL_CRYPTO_DATA_EVENT 1

typedef uint8_t cfil_crypto_key[CCSHA256_OUTPUT_SIZE];
typedef uint8_t cfil_crypto_signature[CFIL_CRYPTO_SIGNATURE_SIZE];

typedef struct cfil_crypto_state {
	const struct ccdigest_info *digest_info;
	cfil_crypto_key key;
} *cfil_crypto_state_t;

typedef struct cfil_crypto_data {
	uuid_t flow_id;
	u_int64_t sock_id;
	u_int32_t direction;
	union sockaddr_in_4_6 remote;
	union sockaddr_in_4_6 local;
	u_int32_t socketProtocol;
	pid_t pid;
	pid_t effective_pid;
	uuid_t uuid;
	uuid_t effective_uuid;
	u_int64_t byte_count_in;
	u_int64_t byte_count_out;
} *cfil_crypto_data_t;

  
                    
  
                                                             
                                          
                                                                 
                                             
   
#define CFM_TYPE_EVENT 1                                 
#define CFM_TYPE_ACTION 2                              

  
                                                
   
#define CFM_OP_SOCKET_ATTACHED 1                                        
#define CFM_OP_SOCKET_CLOSED 2                                        
#define CFM_OP_DATA_OUT 3                                    
#define CFM_OP_DATA_IN 4                                         
#define CFM_OP_DISCONNECT_OUT 5                                    
#define CFM_OP_DISCONNECT_IN 6                                     
#define CFM_OP_STATS 7                                                

  
                                                          
   
#define CFM_OP_DATA_UPDATE 16                                            
#define CFM_OP_DROP 17                                                     
#define CFM_OP_BLESS_CLIENT 18                                                                     
#define CFM_OP_SET_CRYPTO_KEY 19                                                          

  
                      
  
                                
   
struct cfil_msg_hdr {
	uint32_t        cfm_len;                          
	uint32_t        cfm_version;
	uint32_t        cfm_type;
	uint32_t        cfm_op;
	cfil_sock_id_t  cfm_sock_id;
};

#define CFM_VERSION_CURRENT 1

  
                       
   
#define CFS_CONNECTION_DIR_IN  0
#define CFS_CONNECTION_DIR_OUT 1

#define CFS_AUDIT_TOKEN            1

  
                                
  
                                                                      
  
                                                                          
                                                                          
  
                              
  
                                   
   
struct cfil_msg_sock_attached {
	struct cfil_msg_hdr     cfs_msghdr;
	int                     cfs_sock_family;                          
	int                     cfs_sock_type;                                
	int                     cfs_sock_protocol;                            
	int                     cfs_unused;                          
	pid_t                   cfs_pid;
	pid_t                   cfs_e_pid;
	uuid_t                  cfs_uuid;
	uuid_t                  cfs_e_uuid;
	union sockaddr_in_4_6   cfs_src;
	union sockaddr_in_4_6   cfs_dst;
	int                     cfs_conn_dir;
	unsigned int            cfs_audit_token[8];                                           
	cfil_crypto_signature   cfs_signature;
	uint32_t                cfs_signature_length;
};

  
                  
   
#define CFD_DATA_FLAG_IP_HEADER         0x00000001                                       

  
                             
  
                                                       
                                                                    
                                                  
  
                                                                 
                                                             
                                                             
  
                             
  
                                             
   
struct cfil_msg_data_event {
	struct cfil_msg_hdr     cfd_msghdr;
	union sockaddr_in_4_6   cfc_src;
	union sockaddr_in_4_6   cfc_dst;
	uint64_t                cfd_start_offset;
	uint64_t                cfd_end_offset;
	cfil_crypto_signature   cfd_signature;
	uint32_t                cfd_signature_length;
	uint32_t                cfd_flags;
	                                            
};

#define CFI_MAX_TIME_LOG_ENTRY 6
  
                              
  
                                                                
  
                                                                         
          
  
                              
  
                                 
   
struct cfil_msg_sock_closed {
	struct cfil_msg_hdr     cfc_msghdr;
	struct timeval64        cfc_first_event;
	uint32_t                cfc_op_list_ctr;
	uint32_t                cfc_op_time[CFI_MAX_TIME_LOG_ENTRY];                                                         
	unsigned char           cfc_op_list[CFI_MAX_TIME_LOG_ENTRY];
	uint64_t                cfc_byte_inbound_count;
	uint64_t                cfc_byte_outbound_count;
	cfil_crypto_signature   cfc_signature;
	uint32_t                cfc_signature_length;
} __attribute__((aligned(8)));

  
                               
  
                                 
  
                                
  
                              
  
                         
   
struct cfil_msg_sock_stats {
	cfil_sock_id_t          cfs_sock_id;
	uint64_t                cfs_byte_inbound_count;
	uint64_t                cfs_byte_outbound_count;
	union sockaddr_in_4_6   cfs_laddr;
} __attribute__((aligned(8)));

struct cfil_msg_stats_report {
	struct cfil_msg_hdr        cfr_msghdr;
	uint32_t                   cfr_count;
	struct cfil_msg_sock_stats cfr_stats[];
} __attribute__((aligned(8)));

  
                         
  
                              
  
                                             
  
                          
  
                                                                        
                                                                               
  
                                                                              
                                                                            
                                                                          
                                                                  
   
struct cfil_msg_action {
	struct cfil_msg_hdr     cfa_msghdr;
	uint64_t                cfa_in_pass_offset;
	uint64_t                cfa_in_peek_offset;
	uint64_t                cfa_out_pass_offset;
	uint64_t                cfa_out_peek_offset;
	uint32_t                cfa_stats_frequency;                                        
};

  
                               
  
                                                             
  
                              
  
                                 
   
struct cfil_msg_bless_client {
	struct cfil_msg_hdr     cfb_msghdr;
	uuid_t cfb_client_uuid;
};

  
                                 
  
                                                                 
  
                              
  
                                   
   
struct cfil_msg_set_crypto_key {
	struct cfil_msg_hdr     cfb_msghdr;
	cfil_crypto_key         crypto_key;
};

#define CFM_MAX_OFFSET  UINT64_MAX

  
                                     
   
struct cfil_filter_stat {
	uint32_t        cfs_len;
	uint32_t        cfs_filter_id;
	uint32_t        cfs_flags;
	uint32_t        cfs_sock_count;
	uint32_t        cfs_necp_control_unit;
};

struct cfil_entry_stat {
	uint32_t                ces_len;
	uint32_t                ces_filter_id;
	uint32_t                ces_flags;
	uint32_t                ces_necp_control_unit;
	struct timeval64        ces_last_event;
	struct timeval64        ces_last_action;
	struct cfe_buf_stat {
		uint64_t        cbs_pending_first;
		uint64_t        cbs_pending_last;
		uint64_t        cbs_ctl_first;
		uint64_t        cbs_ctl_last;
		uint64_t        cbs_pass_offset;
		uint64_t        cbs_peek_offset;
		uint64_t        cbs_peeked;
	} ces_snd, ces_rcv;
};

struct cfil_sock_stat {
	uint32_t        cfs_len;
	int             cfs_sock_family;
	int             cfs_sock_type;
	int             cfs_sock_protocol;
	cfil_sock_id_t  cfs_sock_id;
	uint64_t        cfs_flags;
	pid_t           cfs_pid;
	pid_t           cfs_e_pid;
	uuid_t          cfs_uuid;
	uuid_t          cfs_e_uuid;
	struct cfi_buf_stat {
		uint64_t        cbs_pending_first;
		uint64_t        cbs_pending_last;
		uint64_t        cbs_pass_offset;
		uint64_t        cbs_inject_q_len;
	} cfs_snd, cfs_rcv;
	struct cfil_entry_stat  ces_entries[CFIL_MAX_FILTER_COUNT];
};

  
                    
   
struct cfil_stats {
	int32_t cfs_ctl_connect_ok;
	int32_t cfs_ctl_connect_fail;
	int32_t cfs_ctl_disconnect_ok;
	int32_t cfs_ctl_disconnect_fail;
	int32_t cfs_ctl_send_ok;
	int32_t cfs_ctl_send_bad;
	int32_t cfs_ctl_rcvd_ok;
	int32_t cfs_ctl_rcvd_bad;
	int32_t cfs_ctl_rcvd_flow_lift;
	int32_t cfs_ctl_action_data_update;
	int32_t cfs_ctl_action_drop;
	int32_t cfs_ctl_action_bad_op;
	int32_t cfs_ctl_action_bad_len;

	int32_t cfs_sock_id_not_found;

	int32_t cfs_cfi_alloc_ok;
	int32_t cfs_cfi_alloc_fail;

	int32_t cfs_sock_userspace_only;
	int32_t cfs_sock_attach_in_vain;
	int32_t cfs_sock_attach_already;
	int32_t cfs_sock_attach_no_mem;
	int32_t cfs_sock_attach_failed;
	int32_t cfs_sock_attached;
	int32_t cfs_sock_detached;

	int32_t cfs_attach_event_ok;
	int32_t cfs_attach_event_flow_control;
	int32_t cfs_attach_event_fail;

	int32_t cfs_closed_event_ok;
	int32_t cfs_closed_event_flow_control;
	int32_t cfs_closed_event_fail;

	int32_t cfs_data_event_ok;
	int32_t cfs_data_event_flow_control;
	int32_t cfs_data_event_fail;

	int32_t cfs_stats_event_ok;
	int32_t cfs_stats_event_flow_control;
	int32_t cfs_stats_event_fail;

	int32_t cfs_disconnect_in_event_ok;
	int32_t cfs_disconnect_out_event_ok;
	int32_t cfs_disconnect_event_flow_control;
	int32_t cfs_disconnect_event_fail;

	int32_t cfs_ctl_q_not_started;

	int32_t cfs_close_wait;
	int32_t cfs_close_wait_timeout;

	int32_t cfs_flush_in_drop;
	int32_t cfs_flush_out_drop;
	int32_t cfs_flush_in_close;
	int32_t cfs_flush_out_close;
	int32_t cfs_flush_in_free;
	int32_t cfs_flush_out_free;

	int32_t cfs_inject_q_nomem;
	int32_t cfs_inject_q_nobufs;
	int32_t cfs_inject_q_detached;
	int32_t cfs_inject_q_in_fail;
	int32_t cfs_inject_q_out_fail;

	int32_t cfs_inject_q_in_retry;
	int32_t cfs_inject_q_out_retry;

	int32_t cfs_data_in_control;
	int32_t cfs_data_in_oob;
	int32_t cfs_data_out_control;
	int32_t cfs_data_out_oob;

	int64_t cfs_ctl_q_in_enqueued __attribute__((aligned(8)));
	int64_t cfs_ctl_q_out_enqueued __attribute__((aligned(8)));
	int64_t cfs_ctl_q_in_peeked __attribute__((aligned(8)));
	int64_t cfs_ctl_q_out_peeked __attribute__((aligned(8)));

	int64_t cfs_pending_q_in_enqueued __attribute__((aligned(8)));
	int64_t cfs_pending_q_out_enqueued __attribute__((aligned(8)));

	int64_t cfs_inject_q_in_enqueued __attribute__((aligned(8)));
	int64_t cfs_inject_q_out_enqueued __attribute__((aligned(8)));
	int64_t cfs_inject_q_in_passed __attribute__((aligned(8)));
	int64_t cfs_inject_q_out_passed __attribute__((aligned(8)));
};
#endif              

#ifdef BSD_KERNEL_PRIVATE

#define M_SKIPCFIL      M_PROTO5

extern int cfil_log_level;

#define CFIL_LOG(level, fmt, ...) \
do { \
	if (cfil_log_level >= level) \
	        printf("%s:%d " fmt "\n",\
	                __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while (0)


extern void cfil_init(void);

extern boolean_t cfil_filter_present(void);
extern boolean_t cfil_sock_connected_pending_verdict(struct socket *so);
extern errno_t cfil_sock_attach(struct socket *so,
    struct sockaddr *local, struct sockaddr *remote, int dir);
extern errno_t cfil_sock_detach(struct socket *so);

extern int cfil_sock_data_out(struct socket *so, struct sockaddr  *to,
    struct mbuf *data, struct mbuf *control,
    uint32_t flags);
extern int cfil_sock_data_in(struct socket *so, struct sockaddr *from,
    struct mbuf *data, struct mbuf *control,
    uint32_t flags);

extern int cfil_sock_shutdown(struct socket *so, int *how);
extern void cfil_sock_is_closed(struct socket *so);
extern void cfil_sock_notify_shutdown(struct socket *so, int how);
extern void cfil_sock_close_wait(struct socket *so);

extern boolean_t cfil_sock_data_pending(struct sockbuf *sb);
extern int cfil_sock_data_space(struct sockbuf *sb);
extern void cfil_sock_buf_update(struct sockbuf *sb);

extern cfil_sock_id_t cfil_sock_id_from_socket(struct socket *so);
extern cfil_sock_id_t cfil_sock_id_from_datagram_socket(struct socket *so, struct sockaddr *local, struct sockaddr *remote);

extern struct m_tag *cfil_dgram_get_socket_state(struct mbuf *m, uint32_t *state_change_cnt,
    uint32_t *options, struct sockaddr **faddr, int *inp_flags);
extern boolean_t cfil_dgram_peek_socket_state(struct mbuf *m, int *inp_flags);

#endif                         

__END_DECLS

#endif                           
