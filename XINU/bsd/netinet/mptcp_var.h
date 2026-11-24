/*
 * Copyright (c) 2012-2020 Apple Inc. All rights reserved.
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

#ifndef _NETINET_MPTCP_VAR_H_
#define _NETINET_MPTCP_VAR_H_

#ifdef PRIVATE
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#ifdef BSD_KERNEL_PRIVATE
#include <sys/queue.h>
#include <sys/protosw.h>
#include <kern/locks.h>
#include <mach/boolean.h>
#include <netinet/mp_pcb.h>
#include <netinet/tcp_var.h>
#include <os/log.h>

struct mpt_itf_info {
	uint32_t ifindex;
	uint32_t has_v4_conn:1,
	    has_v6_conn:1,
	    has_nat64_conn:1,
	    no_mptcp_support:1;
};

  
                
  
                                                                             
                                                 
   
struct mptses {
	struct mppcb    *mpte_mppcb;                                           
	struct mptcb    *mpte_mptcb;                                  
	TAILQ_HEAD(, mptopt) mpte_sopts;                                    
	TAILQ_HEAD(, mptsub) mpte_subflows;                           
#define MPTCP_MAX_NUM_SUBFLOWS 256
	uint16_t        mpte_numflows;                                     
	uint16_t        mpte_nummpcapflows;                               
	sae_associd_t   mpte_associd;                                     
	sae_connid_t    mpte_connid_last;                                    

	uint64_t        mpte_time_target;
	thread_call_t   mpte_time_thread;

	uint32_t        mpte_last_cellicon_set;
	uint32_t        mpte_cellicon_increments;

	union {
		                                       
		struct sockaddr _mpte_src;
		struct sockaddr_in _mpte_src_v4;
		struct sockaddr_in6 _mpte_src_v6;
	} mpte_u_src;
#define mpte_src mpte_u_src._mpte_src
#define __mpte_src_v4 mpte_u_src._mpte_src_v4
#define __mpte_src_v6 mpte_u_src._mpte_src_v6
	union {
		                                            
		struct sockaddr _mpte_dst;
		struct sockaddr_in _mpte_dst_v4;
		struct sockaddr_in6 _mpte_dst_v6;
	} mpte_u_dst;
#define mpte_dst mpte_u_dst._mpte_dst
#define __mpte_dst_v4 mpte_u_dst._mpte_dst_v4
#define __mpte_dst_v6 mpte_u_dst._mpte_dst_v6

	struct sockaddr_in      mpte_sub_dst_v4;
	struct sockaddr_in6     mpte_sub_dst_v6;

	uint16_t        mpte_alternate_port;                                                                       

	int mpte_epid;
	uuid_t mpte_euuid;

	struct mptsub   *mpte_active_sub;                                    
	uint16_t mpte_flags;                                                 
#define MPTE_SND_REM_ADDR       0x01                                         
#define MPTE_SVCTYPE_CHECKED    0x02                                                        
#define MPTE_FIRSTPARTY         0x04                                                                     
#define MPTE_ACCESS_GRANTED     0x08                                                                         
#define MPTE_FORCE_ENABLE       0x10                                                                          
#define MPTE_IN_WORKLOOP        0x20                                                        
#define MPTE_WORKLOOP_RELAUNCH  0x40                                                                          
#define MPTE_UNICAST_IP         0x80                                                                                                
#define MPTE_CELL_PROHIBITED    0x100                                                                        
	uint8_t mpte_svctype;                                           
	uint8_t mpte_lost_aid;                                               
	uint8_t mpte_addrid_last;                                            

#define MPTE_ITFINFO_SIZE       4
	uint32_t        mpte_itfinfo_size;
	struct mpt_itf_info     _mpte_itfinfo[MPTE_ITFINFO_SIZE];
	struct mpt_itf_info     *mpte_itfinfo;

	struct mbuf             *mpte_reinjectq;

	                                 
	uint32_t        mpte_subflow_switches;                                             
	uint32_t        mpte_used_cell:1,
	    mpte_used_wifi:1,
	    mpte_initial_cell:1,
	    mpte_triggered_cell,
	    mpte_handshake_success:1;

	struct mptcp_itf_stats  mpte_itfstats[MPTCP_ITFSTATS_SIZE];
	uint64_t                mpte_init_txbytes __attribute__((aligned(8)));
	uint64_t                mpte_init_rxbytes __attribute__((aligned(8)));
};

static inline struct socket *
mptetoso(struct mptses *mpte)
{
	return mpte->mpte_mppcb->mpp_socket;
}

static inline struct mptses *
mptompte(struct mppcb *mp)
{
	return (struct mptses *)mp->mpp_pcbe;
}

static inline struct mptses *
mpsotompte(struct socket *so)
{
	return mptompte(mpsotomppcb(so));
}

static inline boolean_t
mpp_try_lock(struct mppcb *mp)
{
	if (!lck_mtx_try_lock(&mp->mpp_lock)) {
		return false;
	}

	VERIFY(!(mp->mpp_flags & MPP_INSIDE_OUTPUT));
	VERIFY(!(mp->mpp_flags & MPP_INSIDE_INPUT));

	return true;
}

static inline void
mpp_lock(struct mppcb *mp)
{
	lck_mtx_lock(&mp->mpp_lock);
	VERIFY(!(mp->mpp_flags & MPP_INSIDE_OUTPUT));
	VERIFY(!(mp->mpp_flags & MPP_INSIDE_INPUT));
}

static inline void
mpp_unlock(struct mppcb *mp)
{
	VERIFY(!(mp->mpp_flags & MPP_INSIDE_OUTPUT));
	VERIFY(!(mp->mpp_flags & MPP_INSIDE_INPUT));
	lck_mtx_unlock(&mp->mpp_lock);
}

static inline lck_mtx_t *
mpp_getlock(struct mppcb *mp, int flags)
{
	if (flags & PR_F_WILLUNLOCK) {
		VERIFY(!(mp->mpp_flags & MPP_INSIDE_OUTPUT));
		VERIFY(!(mp->mpp_flags & MPP_INSIDE_INPUT));
	}

	return &mp->mpp_lock;
}

static inline int
mptcp_subflow_cwnd_space(struct socket *so)
{
	struct tcpcb *tp = sototcpcb(so);
	int cwnd = (int)(MIN(tp->snd_wnd, tp->snd_cwnd) - (so->so_snd.sb_cc));

	return MIN(cwnd, sbspace(&so->so_snd));
}

static inline bool
mptcp_subflows_need_backup_flag(struct mptses *mpte)
{
	return mpte->mpte_svctype < MPTCP_SVCTYPE_AGGREGATE ||
	       mpte->mpte_svctype == MPTCP_SVCTYPE_PURE_HANDOVER;
}

  
                       
   
struct mptopt {
	TAILQ_ENTRY(mptopt)     mpo_entry;                                 
	uint32_t                mpo_flags;                           
	int                     mpo_level;                      
	int                     mpo_name;                      
	int                     mpo_intval;                   
};

#define MPOF_ATTACHED           0x1                                
#define MPOF_SUBFLOW_OK         0x2                                          
#define MPOF_INTERIM            0x4                                             

  
                
  
                                                                          
   
struct mptsub {
	TAILQ_ENTRY(mptsub)   mpts_entry;                                
	uint32_t              mpts_refcnt;                         
	uint32_t              mpts_flags;                          
	long                  mpts_evctl;                                 
	sae_connid_t          mpts_connid;                               
	int                   mpts_oldintval;                                
	struct mptses         *mpts_mpte;                                    
	struct socket         *mpts_socket;                       
	struct sockaddr       *mpts_src;                          

	union {
		                         
		struct sockaddr         _mpts_dst;
		struct sockaddr_in      _mpts_dst_v4;
		struct sockaddr_in6     _mpts_dst_v6;
	} mpts_u_dst;
#define mpts_dst mpts_u_dst._mpts_dst
#define __mpts_dst_v4 mpts_u_dst._mpts_dst_v4
#define __mpts_dst_v6 mpts_u_dst._mpts_dst_v6
	u_int32_t               mpts_rel_seq;                                   
	u_int32_t               mpts_iss;                                                             
	u_int32_t               mpts_ifscope;                                
	uint32_t                mpts_probesoon;                               
	uint32_t                mpts_probecnt;                             
	uint32_t                mpts_maxseg;                                  
};

  
                                               
  
                                                                           
                                                                         
                                                                           
                                                                        
                                                                        
                                                                      
                 
  
                                                                            
                                         
  
                                                                               
                                                                     
                                                                         
                                        
  
                                                                               
                                                                           
                                                                              
                                                                         
                                                                           
                                                                             
                                                                          
  
                                                    
   
#define MPTSF_ATTACHED          0x00000001                                 
#define MPTSF_CONNECTING        0x00000002                                    
#define MPTSF_CONNECT_PENDING   0x00000004                                            
#define MPTSF_CONNECTED         0x00000008                                     
#define MPTSF_DISCONNECTING     0x00000010                                       
#define MPTSF_DISCONNECTED      0x00000020                                 
#define MPTSF_MP_CAPABLE        0x00000040                                        
#define MPTSF_MP_READY          0x00000080                                    
#define MPTSF_MP_DEGRADED       0x00000100                                           
#define MPTSF_PREFERRED         0x00000200                                     
#define MPTSF_SOPT_OLDVAL       0x00000400                                     
#define MPTSF_SOPT_INPROG       0x00000800                                
#define MPTSF_FAILINGOVER       0x00001000                                       
#define MPTSF_ACTIVE            0x00002000                                    
#define MPTSF_MPCAP_CTRSET      0x00004000                         
#define MPTSF_CLOSED            0x00008000                                                          
#define MPTSF_TFO_REQD          0x00010000                         
#define MPTSF_CLOSE_REQD        0x00020000                                                
#define MPTSF_INITIAL_SUB       0x00040000                                       
#define MPTSF_READ_STALL        0x00080000                                          
#define MPTSF_WRITE_STALL       0x00100000                                           
#define MPTSF_FULLY_ESTABLISHED 0x00200000                                                               
                                                                             
                                                                                    
                                            
#define MPTSF_CELLICON_SET      0x00400000                                         

#define MPTSF_BITS \
	"\020\1ATTACHED\2CONNECTING\3PENDING\4CONNECTED\5DISCONNECTING" \
	"\6DISCONNECTED\7MP_CAPABLE\10MP_READY\11MP_DEGRADED" \
	"\12PREFERRED\13SOPT_OLDVAL" \
	"\14SOPT_INPROG\15FAILINGOVER\16ACTIVE\17MPCAP_CTRSET" \
	"\20CLOSED\21TFO_REQD\22CLOSEREQD\23INITIALSUB\24READ_STALL" \
	"\25WRITE_STALL\26CONFIRMED"

  
               
                                           
   
typedef enum mptcp_state {
	MPTCPS_CLOSED           = 0,                
	MPTCPS_LISTEN           = 1,                             
	MPTCPS_ESTABLISHED      = 2,                                      
	MPTCPS_CLOSE_WAIT       = 3,                                      
	MPTCPS_FIN_WAIT_1       = 4,                                
	MPTCPS_CLOSING          = 5,                                            
	MPTCPS_LAST_ACK         = 6,                                            
	MPTCPS_FIN_WAIT_2       = 7,                                    
	MPTCPS_TIME_WAIT        = 8,                                         
	MPTCPS_TERMINATE        = 9,                        
} mptcp_state_t;

typedef u_int64_t       mptcp_key_t;
typedef u_int32_t       mptcp_token_t;
typedef u_int8_t        mptcp_addr_id;


                     
struct mptcp_subf_auth_entry {
	LIST_ENTRY(mptcp_subf_auth_entry) msae_next;
	u_int32_t       msae_laddr_rand;                         
	u_int32_t       msae_raddr_rand;                          
	mptcp_addr_id   msae_laddr_id;                             
	mptcp_addr_id   msae_raddr_id;                              
};

  
                               
  
                                   
                                                    
   
struct mptcb {
	struct mptses  *mpt_mpte;                                              
	mptcp_state_t   mpt_state;                               
	uint32_t        mpt_flags;                                   
	uint8_t         mpt_version;                                     
	uint8_t         mpt_peer_version;                              
	u_short         mpt_softerror;                                      
	  
                                          
    
	mptcp_key_t     mpt_localkey;                                      
	mptcp_key_t     mpt_remotekey;                                     
	mptcp_token_t   mpt_localtoken;                                     
	mptcp_token_t   mpt_remotetoken;                                     

	  
                                                                 
                     
    
	int             mpt_rxtshift;                                           
	uint64_t        mpt_rxtstart;                                          
	uint64_t        mpt_rtseq;                                       
	uint64_t        mpt_timewait;                         
	uint32_t        mpt_timer_vals;                                   
	  
                
    
	uint64_t        mpt_snduna;                                           
	uint64_t        mpt_sndnxt;                                           
	uint64_t        mpt_sndmax;                                       
	uint64_t        mpt_local_idsn;                               
	uint32_t        mpt_sndwnd;
	uint64_t        mpt_sndwl1;
	uint64_t        mpt_sndwl2;
	  
                  
    
	uint64_t        mpt_rcvnxt;                                    
	uint64_t        mpt_remote_idsn;                         
	uint64_t        mpt_rcvadv;
	uint32_t        mpt_rcvwnd;
	LIST_HEAD(, mptcp_subf_auth_entry) mpt_subauth_list;                  
	  
             
    
	uint64_t        mpt_dsn_at_csum_fail;                       
	uint32_t        mpt_ssn_at_csum_fail;                           
	  
                   
    
#define MPT_GC_TICKS            (30)
#define MPT_GC_TICKS_FAST       (10)
	int32_t         mpt_gc_ticks;                                         

	uint32_t        mpt_notsent_lowat;                                     

	struct tsegqe_head      mpt_segq;
	uint32_t        mpt_reassqlen;                                          
};

                                                                     
#define MPTCPF_CHECKSUM                 0x001                            
#define MPTCPF_FALLBACK_TO_TCP          0x002                        
#define MPTCPF_JOIN_READY               0x004                                          
#define MPTCPF_RECVD_MPFAIL             0x008                                
#define MPTCPF_SND_64BITDSN             0x010                             
#define MPTCPF_SND_64BITACK             0x020                                 
#define MPTCPF_RCVD_64BITACK            0x040                                 
#define MPTCPF_POST_FALLBACK_SYNC       0x080                                  
#define MPTCPF_FALLBACK_HEURISTIC       0x100                                                     
#define MPTCPF_HEURISTIC_TRAC           0x200                                                               
#define MPTCPF_REASS_INPROG             0x400                                  
#define MPTCPF_UNICAST_IP               0x800

#define MPTCPF_BITS \
	"\020\1CHECKSUM\2FALLBACK_TO_TCP\3JOIN_READY\4RECVD_MPFAIL" \
	"\5SND_64BITDSN\6SND_64BITACK\7RCVD_64BITACK\10POST_FALLBACK_SYNC" \
	"\11FALLBACK_HEURISTIC\12HEURISTIC_TRAC\13REASS_INPROG"

                                     
#define MPTT_REXMT              0x01                                   
#define MPTT_TW                 0x02                                 
#define MPTT_FASTCLOSE          0x04                                       

                          
#define MPCE_CLOSE              0x1
#define MPCE_RECV_DATA_ACK      0x2
#define MPCE_RECV_DATA_FIN      0x4

                        
static inline struct mptcb *
tptomptp(struct tcpcb *tp)
{
	return tp->t_mptcb;
}

  
                                                                    
                                                                       
   
struct mpp_mtp {
	struct mppcb            mpp;                               
	struct mptses           mpp_ses;                           
	struct mptcb            mtcb;                          
};

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_mptcp);
#endif                  

extern struct mppcbinfo mtcbinfo;
extern struct pr_usrreqs mptcp_usrreqs;
extern os_log_t mptcp_log_handle;

                                              
#define SHA1_TRUNCATED          8

                            
#define MPTCP_LOGLVL_NONE       0x0                           
#define MPTCP_LOGLVL_ERR        0x1                                         
#define MPTCP_LOGLVL_LOG        0x2                         
#define MPTCP_LOGLVL_VERBOSE    0x4                       

                                            
#define MPTCP_NO_DBG            0x00                             
#define MPTCP_STATE_DBG         0x01                               
#define MPTCP_SOCKET_DBG        0x02                             
#define MPTCP_SENDER_DBG        0x04                             
#define MPTCP_RECEIVER_DBG      0x08                          
#define MPTCP_EVENTS_DBG        0x10                                

                                                           
#define MPTCP_DATASEQ_LOW32_MASK        (0xffffffff)
#define MPTCP_DATASEQ_LOW32(seq)        (seq & MPTCP_DATASEQ_LOW32_MASK)

                                                                 
#define MPTCP_DATASEQ_HIGH32_MASK       (0xffffffff00000000)
#define MPTCP_DATASEQ_HIGH32(seq)       (seq & MPTCP_DATASEQ_HIGH32_MASK)

                                               
#define MPTCP_DATAACK_LOW32_MASK        (0xffffffff)
#define MPTCP_DATAACK_LOW32(ack)        (ack & MPTCP_DATAACK_LOW32_MASK)

                                                     
#define MPTCP_DATAACK_HIGH32_MASK       (0xffffffff00000000)
#define MPTCP_DATAACK_HIGH32(ack)       (ack & MPTCP_DATAACK_HIGH32_MASK)

  
                                                                           
                                                             
                                                                          
                                                                              
                                                                            
                                                                         
                                                                           
                                                                           
                                                                         
                                                                       
                                                                             
                                                 
   
#define MPTCP_EXTEND_DSN(x, y, z) {                                     \
	if ((MPTCP_DATASEQ_LOW32(x) > y) &&                             \
	    ((((u_int32_t)MPTCP_DATASEQ_LOW32(x)) - (u_int32_t)y) >=    \
	    (u_int32_t)(1U << 31))) {                                    \
	    
                                                         
                                                                 \
	        z = MPTCP_DATASEQ_HIGH32(x) + 0x100000000;              \
	        z |= y;                                                 \
	} else if ((MPTCP_DATASEQ_LOW32(x) < y) &&                      \
	    (((u_int32_t)y -                                            \
	    ((u_int32_t)MPTCP_DATASEQ_LOW32(x))) >=                     \
	    (u_int32_t)(1U << 31))) {                                    \
	    
                                                  
                                                                 \
	        z = MPTCP_DATASEQ_HIGH32(x) - 0x100000000;              \
	        z |= y;                                                 \
	} else {                                                        \
	        z = MPTCP_DATASEQ_HIGH32(x) | y;                        \
	}                                                               \
}

#define mptcplog(x, y, z)       do {                                    \
	if ((mptcp_dbg_area & y) && (mptcp_dbg_level & z))              \
	        log x;                                                  \
} while (0)

extern int mptcp_enable;                           
extern int mptcp_mpcap_retries;                            
extern int mptcp_join_retries;                                  
extern int mptcp_dss_csum;                                         
extern int mptcp_fail_thresh;                                                 
extern int mptcp_subflow_keeptime;                                          
extern uint32_t mptcp_dbg_level;                                           
extern uint32_t mptcp_dbg_area;                                   
extern int mptcp_developer_mode;                                    
extern uint32_t mptcp_cellicon_refcount;

#define MPTCP_CELLICON_TOGGLE_RATE      (5 * TCP_RETRANSHZ)                                  

extern int tcp_jack_rxmt;                                                   

__BEGIN_DECLS
extern void mptcp_init(struct protosw *, struct domain *);
extern int mptcp_ctloutput(struct socket *, struct sockopt *);
extern int mptcp_session_create(struct mppcb *);
extern boolean_t mptcp_ok_to_create_subflows(struct mptcb *mp_tp);
extern void mptcp_check_subflows_and_add(struct mptses *mpte);
extern void mptcp_check_subflows_and_remove(struct mptses *mpte);
extern void mptcpstats_inc_switch(struct mptses *mpte, const struct mptsub *mpts);
extern void mptcpstats_update(struct mptcp_itf_stats *stats, const struct mptsub *mpts);
extern int mptcpstats_get_index_by_ifindex(struct mptcp_itf_stats *stats, u_short ifindex, boolean_t create);
extern struct mptses *mptcp_drop(struct mptses *mpte, struct mptcb *mp_tp, u_short errno);
extern struct mptses *mptcp_close(struct mptses *, struct mptcb *);
extern int mptcp_lock(struct socket *, int, void *);
extern int mptcp_unlock(struct socket *, int, void *);
extern lck_mtx_t *mptcp_getlock(struct socket *, int);
extern void mptcp_subflow_workloop(struct mptses *);

extern void mptcp_sched_create_subflows(struct mptses *);

extern void mptcp_finish_usrclosed(struct mptses *mpte);
extern struct mptopt *mptcp_sopt_alloc(zalloc_flags_t);
extern const char *mptcp_sopt2str(int, int);
extern void mptcp_sopt_free(struct mptopt *);
extern void mptcp_sopt_insert(struct mptses *, struct mptopt *);
extern void mptcp_sopt_remove(struct mptses *, struct mptopt *);
extern struct mptopt *mptcp_sopt_find(struct mptses *, struct sockopt *);

extern int mptcp_subflow_add(struct mptses *, struct sockaddr *,
    struct sockaddr *, uint32_t, sae_connid_t *);
extern void mptcp_subflow_del(struct mptses *, struct mptsub *);

extern void mptcp_handle_input(struct socket *so);
#define MPTCP_SUBOUT_PROBING    0x01
extern int mptcp_subflow_output(struct mptses *mpte, struct mptsub *mpts, int flags);
extern void mptcp_clean_reinjectq(struct mptses *mpte);
extern void mptcp_subflow_shutdown(struct mptses *, struct mptsub *);
extern void mptcp_subflow_disconnect(struct mptses *, struct mptsub *);
extern int mptcp_subflow_sosetopt(struct mptses *, struct mptsub *,
    struct mptopt *);
extern int mptcp_subflow_sogetopt(struct mptses *, struct socket *,
    struct mptopt *);

extern void mptcp_input(struct mptses *, struct mbuf *);
extern boolean_t mptcp_can_send_more(struct mptcb *mp_tp, boolean_t ignore_reinject);
extern int mptcp_output(struct mptses *);
extern void mptcp_close_fsm(struct mptcb *, uint32_t);

extern void mptcp_hmac_sha1(mptcp_key_t, mptcp_key_t, u_int32_t, u_int32_t,
    u_char*);
extern void mptcp_get_hmac(mptcp_addr_id, struct mptcb *, u_char *);
extern void mptcp_get_rands(mptcp_addr_id, struct mptcb *, u_int32_t *,
    u_int32_t *);
extern void mptcp_set_raddr_rand(mptcp_addr_id, struct mptcb *, mptcp_addr_id,
    u_int32_t);
extern int mptcp_init_remote_parms(struct mptcb *);
extern boolean_t mptcp_ok_to_keepalive(struct mptcb *);
extern void mptcp_insert_dsn(struct mppcb *, struct mbuf *);
extern void mptcp_output_getm_dsnmap32(struct socket *so, int off,
    uint32_t *dsn, uint32_t *relseq,
    uint16_t *data_len, uint16_t *dss_csum);
extern void mptcp_output_getm_dsnmap64(struct socket *so, int off,
    uint64_t *dsn, uint32_t *relseq,
    uint16_t *data_len, uint16_t *dss_csum);
extern void mptcp_act_on_txfail(struct socket *);
extern struct mptsub *mptcp_get_subflow(struct mptses *mpte, struct mptsub **preferred);
extern int mptcp_get_map_for_dsn(struct socket *so, uint64_t dsn_fail, uint32_t *tcp_seq);
extern int32_t mptcp_adj_sendlen(struct socket *so, int32_t off);
extern void mptcp_sbrcv_grow(struct mptcb *mp_tp);
extern int32_t mptcp_sbspace(struct mptcb *);
extern void mptcp_notify_mpready(struct socket *);
extern void mptcp_notify_mpfail(struct socket *);
extern void mptcp_notify_close(struct socket *);
extern boolean_t mptcp_no_rto_spike(struct socket*);
extern int mptcp_set_notsent_lowat(struct mptses *mpte, int optval);
extern u_int32_t mptcp_get_notsent_lowat(struct mptses *mpte);
extern int mptcp_notsent_lowat_check(struct socket *so);
extern void mptcp_ask_symptoms(struct mptses *mpte);
extern void mptcp_control_register(void);
extern int mptcp_is_wifi_unusable_for_session(struct mptses *mpte);
extern boolean_t symptoms_is_wifi_lossy(void);
extern void mptcp_session_necp_cb(void *, int, uint32_t, uint32_t, bool *);
extern struct sockaddr *mptcp_get_session_dst(struct mptses *mpte,
    boolean_t has_v6, boolean_t has_v4);
extern void mptcp_set_restrictions(struct socket *mp_so);
extern void mptcp_clear_cellicon(void);
extern void mptcp_unset_cellicon(struct mptses *mpte, struct mptsub *mpts, uint32_t val);
extern void mptcp_reset_rexmit_state(struct tcpcb *tp);
extern void mptcp_reset_keepalive(struct tcpcb *tp);
extern int mptcp_validate_csum(struct tcpcb *tp, struct mbuf *m, uint64_t dsn,
    uint32_t sseq, uint16_t dlen, uint16_t csum, int dfin);
__END_DECLS

#endif                         
#ifdef PRIVATE

typedef struct mptcp_flow {
	uint64_t                flow_len;
	uint64_t                flow_tcpci_offset;
	uint32_t                flow_flags;
	sae_connid_t            flow_cid;
	struct sockaddr_storage flow_src;
	struct sockaddr_storage flow_dst;
	uint32_t                flow_relseq;                               
	int32_t                 flow_soerror;                            
	uint32_t                flow_probecnt;                             
	conninfo_tcp_t          flow_ci;                                    
} mptcp_flow_t;

typedef struct conninfo_mptcp {
	uint64_t        mptcpci_len;
	uint64_t        mptcpci_flow_offset;                             
	uint64_t        mptcpci_nflows;                                 
	uint32_t        mptcpci_state;                                 
	uint32_t        mptcpci_mpte_flags;                        
	uint32_t        mptcpci_flags;                           
	uint32_t        mptcpci_ltoken;                          
	uint32_t        mptcpci_rtoken;                           
	uint32_t        mptcpci_notsent_lowat;                     

	               
	uint64_t        mptcpci_snduna;                                       
	uint64_t        mptcpci_sndnxt;                                       
	uint64_t        mptcpci_sndmax;                                   
	uint64_t        mptcpci_lidsn;                          
	uint32_t        mptcpci_sndwnd;                                   

	                  
	uint64_t        mptcpci_rcvnxt;                                
	uint64_t        mptcpci_rcvatmark;                                
	uint64_t        mptcpci_ridsn;                           
	uint32_t        mptcpci_rcvwnd;                             

	uint8_t         mptcpci_mpte_addrid;                      

	mptcp_flow_t    mptcpci_flows[1];
} conninfo_mptcp_t;

                                                                              
#define MPTCP_KERN_CTL_NAME    "com.apple.network.advisory"
typedef struct symptoms_advisory {
	union {
		uint32_t        sa_nwk_status_int;
		struct {
			union {
#define SYMPTOMS_ADVISORY_NOCOMMENT     0x0000
#define SYMPTOMS_ADVISORY_USEAPP        0xFFFF                                                                                          
				uint16_t        sa_nwk_status;
				struct {
#define SYMPTOMS_ADVISORY_WIFI_BAD     0x01
#define SYMPTOMS_ADVISORY_WIFI_OK      0x02
					uint8_t sa_wifi_status;
#define SYMPTOMS_ADVISORY_CELL_BAD     0x01
#define SYMPTOMS_ADVISORY_CELL_OK      0x02
					uint8_t sa_cell_status;
				};
			};
			uint16_t        sa_unused;
		};
	};
} symptoms_advisory_t;

#define MPTCP_TARGET_BASED_RSSI_THRESHOLD -75
struct mptcp_symptoms_answer {
	struct symptoms_advisory advisory;
	uuid_t  uuid;
	int32_t rssi;
};

struct mptcp_symptoms_ask_uuid {
	uint32_t        cmd;
#define MPTCP_SYMPTOMS_ASK_UUID         1
	uuid_t          uuid;
	uint32_t        priority;
#define MPTCP_SYMPTOMS_UNKNOWN          0
#define MPTCP_SYMPTOMS_BACKGROUND       1
#define MPTCP_SYMPTOMS_FOREGROUND       2
};

struct kev_mptcp_data {
	int value;
};

#endif              
#endif                            
