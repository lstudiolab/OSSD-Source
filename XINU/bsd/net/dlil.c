/*
 * Copyright (c) 1999-2021 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   
#include <stddef.h>
#include <ptrauth.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/domain.h>
#include <sys/user.h>
#include <sys/random.h>
#include <sys/socketvar.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_var.h>
#include <net/dlil.h>
#include <net/if_arp.h>
#include <net/iptap.h>
#include <net/pktap.h>
#include <sys/kern_event.h>
#include <sys/kdebug.h>
#include <sys/mcache.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/priv.h>

#include <kern/assert.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/locks.h>
#include <kern/zalloc.h>

#include <net/kpi_protocol.h>
#include <net/if_types.h>
#include <net/if_ipsec.h>
#include <net/if_llreach.h>
#include <net/if_utun.h>
#include <net/kpi_interfacefilter.h>
#include <net/classq/classq.h>
#include <net/classq/classq_sfb.h>
#include <net/flowhash.h>
#include <net/ntstat.h>
#include <net/if_llatbl.h>
#include <net/net_api_stats.h>
#include <net/if_ports_used.h>
#include <net/if_vlan_var.h>
#include <netinet/in.h>
#if INET
#include <netinet/in_var.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/if_ether.h>
#include <netinet/in_pcb.h>
#include <netinet/in_tclass.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#endif           

#include <net/nat464_utils.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/pf_pbuf.h>
#include <libkern/OSAtomic.h>
#include <libkern/tree.h>

#include <dev/random/randomdev.h>
#include <machine/machine_routines.h>

#include <mach/thread_act.h>
#include <mach/sdt.h>

#if CONFIG_MACF
#include <sys/kauth.h>
#include <security/mac_framework.h>
#include <net/ethernet.h>
#include <net/firewire.h>
#endif

#if PF
#include <net/pfvar.h>
#endif         
#include <net/pktsched/pktsched.h>
#include <net/pktsched/pktsched_netem.h>

#if NECP
#include <net/necp.h>
#endif           


#include <os/log.h>

#define DBG_LAYER_BEG           DLILDBG_CODE(DBG_DLIL_STATIC, 0)
#define DBG_LAYER_END           DLILDBG_CODE(DBG_DLIL_STATIC, 2)
#define DBG_FNC_DLIL_INPUT      DLILDBG_CODE(DBG_DLIL_STATIC, (1 << 8))
#define DBG_FNC_DLIL_OUTPUT     DLILDBG_CODE(DBG_DLIL_STATIC, (2 << 8))
#define DBG_FNC_DLIL_IFOUT      DLILDBG_CODE(DBG_DLIL_STATIC, (3 << 8))

#define MAX_FRAME_TYPE_SIZE 4                
#define MAX_LINKADDR        4                
#define M_NKE M_IFADDR

#if 1
#define DLIL_PRINTF     printf
#else
#define DLIL_PRINTF     kprintf
#endif

#define IF_DATA_REQUIRE_ALIGNED_64(f)   \
	_CASSERT(!(offsetof(struct if_data_internal, f) % sizeof (u_int64_t)))

#define IFNET_IF_DATA_REQUIRE_ALIGNED_64(f)     \
	_CASSERT(!(offsetof(struct ifnet, if_data.f) % sizeof (u_int64_t)))

enum {
	kProtoKPI_v1    = 1,
	kProtoKPI_v2    = 2
};

  
                                                                 
                                                                      
                                                                 
                                                  
   
struct if_proto {
	SLIST_ENTRY(if_proto)       next_hash;
	u_int32_t                   refcount;
	u_int32_t                   detached;
	struct ifnet                *ifp;
	protocol_family_t           protocol_family;
	int                         proto_kpi;
	union {
		struct {
			proto_media_input               input;
			proto_media_preout              pre_output;
			proto_media_event               event;
			proto_media_ioctl               ioctl;
			proto_media_detached            detached;
			proto_media_resolve_multi       resolve_multi;
			proto_media_send_arp            send_arp;
		} v1;
		struct {
			proto_media_input_v2            input;
			proto_media_preout              pre_output;
			proto_media_event               event;
			proto_media_ioctl               ioctl;
			proto_media_detached            detached;
			proto_media_resolve_multi       resolve_multi;
			proto_media_send_arp            send_arp;
		} v2;
	} kpi;
};

SLIST_HEAD(proto_hash_entry, if_proto);

#define DLIL_SDLDATALEN \
	(DLIL_SDLMAXLEN - offsetof(struct sockaddr_dl, sdl_data[0]))

struct dlil_ifnet {
	struct ifnet    dl_if;                                    
	  
                                                
    
	decl_lck_mtx_data(, dl_if_lock);
	TAILQ_ENTRY(dlil_ifnet) dl_if_link;                          
	u_int32_t dl_if_flags;                                     
	u_int32_t dl_if_refcnt;                             
	void (*dl_if_trace)(struct dlil_ifnet *, int);                         
	void    *dl_if_uniqueid;                                         
	size_t  dl_if_uniqueid_len;                                          
	char    dl_if_namestorage[IFNAMSIZ];                                
	char    dl_if_xnamestorage[IFXNAMSIZ];                             
	struct {
		struct ifaddr   ifa;                            
		u_int8_t        asdl[DLIL_SDLMAXLEN];                   
		u_int8_t        msdl[DLIL_SDLMAXLEN];                   
	} dl_if_lladdr;
	u_int8_t dl_if_descstorage[IF_DESCSIZE];                   
	u_int8_t dl_if_permanent_ether[ETHER_ADDR_LEN];                        
	u_int8_t dl_if_permanent_ether_is_set;
	u_int8_t dl_if_unused;
	struct dlil_threading_info dl_if_inpstorage;                           
	ctrace_t        dl_if_attach;                                     
	ctrace_t        dl_if_detach;                                     
};

                                              
#define DLIF_INUSE      0x1                                            
#define DLIF_REUSE      0x2                                                
#define DLIF_DEBUG      0x4                             

#define IF_REF_TRACE_HIST_SIZE  8                                      

             
__private_extern__ unsigned int if_ref_trace_hist_size = IF_REF_TRACE_HIST_SIZE;

struct dlil_ifnet_dbg {
	struct dlil_ifnet       dldbg_dlif;                             
	u_int16_t               dldbg_if_refhold_cnt;                           
	u_int16_t               dldbg_if_refrele_cnt;                         
	  
                                                        
    
	ctrace_t                dldbg_if_refhold[IF_REF_TRACE_HIST_SIZE];
	ctrace_t                dldbg_if_refrele[IF_REF_TRACE_HIST_SIZE];
};

#define DLIL_TO_IFP(s)  (&s->dl_if)
#define IFP_TO_DLIL(s)  ((struct dlil_ifnet *)s)

struct ifnet_filter {
	TAILQ_ENTRY(ifnet_filter)       filt_next;
	u_int32_t                       filt_skip;
	u_int32_t                       filt_flags;
	ifnet_t                         filt_ifp;
	const char                      *filt_name;
	void                            *filt_cookie;
	protocol_family_t               filt_protocol;
	iff_input_func                  filt_input;
	iff_output_func                 filt_output;
	iff_event_func                  filt_event;
	iff_ioctl_func                  filt_ioctl;
	iff_detached_func               filt_detached;
};

struct proto_input_entry;

static TAILQ_HEAD(, dlil_ifnet) dlil_ifnet_head;
static lck_grp_t *dlil_lock_group;
lck_grp_t *ifnet_lock_group;
static lck_grp_t *ifnet_head_lock_group;
static lck_grp_t *ifnet_snd_lock_group;
static lck_grp_t *ifnet_rcv_lock_group;
lck_attr_t *ifnet_lock_attr;
decl_lck_rw_data(static, ifnet_head_lock);
decl_lck_mtx_data(static, dlil_ifnet_lock);
u_int32_t dlil_filter_disable_tso_count = 0;

#if DEBUG
static unsigned int ifnet_debug = 1;                             
#else
static unsigned int ifnet_debug;                                  
#endif             
static unsigned int dlif_size;                                              
static unsigned int dlif_bufsize;                                         
static struct zone *dlif_zone;                                   
#define DLIF_ZONE_NAME          "ifnet"                        

static ZONE_DECLARE(dlif_filt_zone, "ifnet_filter",
    sizeof(struct ifnet_filter), ZC_ZFREE_CLEARMEM);

static ZONE_DECLARE(dlif_phash_zone, "ifnet_proto_hash",
    sizeof(struct proto_hash_entry) * PROTO_HASH_SLOTS, ZC_ZFREE_CLEARMEM);

static ZONE_DECLARE(dlif_proto_zone, "ifnet_proto",
    sizeof(struct if_proto), ZC_ZFREE_CLEARMEM);

static unsigned int dlif_tcpstat_size;                                         
static unsigned int dlif_tcpstat_bufsize;                                           
static struct zone *dlif_tcpstat_zone;                                      
#define DLIF_TCPSTAT_ZONE_NAME  "ifnet_tcpstat"                

static unsigned int dlif_udpstat_size;                                         
static unsigned int dlif_udpstat_bufsize;                                                 
static struct zone *dlif_udpstat_zone;                                      
#define DLIF_UDPSTAT_ZONE_NAME  "ifnet_udpstat"                

static u_int32_t net_rtref;

static struct dlil_main_threading_info dlil_main_input_thread_info;
__private_extern__ struct dlil_threading_info *dlil_main_input_thread =
    (struct dlil_threading_info *)&dlil_main_input_thread_info;

static int dlil_event_internal(struct ifnet *ifp, struct kev_msg *msg, bool update_generation);
static int dlil_detach_filter_internal(interface_filter_t filter, int detached);
static void dlil_if_trace(struct dlil_ifnet *, int);
static void if_proto_ref(struct if_proto *);
static void if_proto_free(struct if_proto *);
static struct if_proto *find_attached_proto(struct ifnet *, u_int32_t);
static u_int32_t dlil_ifp_protolist(struct ifnet *ifp, protocol_family_t *list,
    u_int32_t list_count);
static void if_flt_monitor_busy(struct ifnet *);
static void if_flt_monitor_unbusy(struct ifnet *);
static void if_flt_monitor_enter(struct ifnet *);
static void if_flt_monitor_leave(struct ifnet *);
static int dlil_interface_filters_input(struct ifnet *, struct mbuf **,
    char **, protocol_family_t);
static int dlil_interface_filters_output(struct ifnet *, struct mbuf **,
    protocol_family_t);
static struct ifaddr *dlil_alloc_lladdr(struct ifnet *,
    const struct sockaddr_dl *);
static int ifnet_lookup(struct ifnet *);
static void if_purgeaddrs(struct ifnet *);

static errno_t ifproto_media_input_v1(struct ifnet *, protocol_family_t,
    struct mbuf *, char *);
static errno_t ifproto_media_input_v2(struct ifnet *, protocol_family_t,
    struct mbuf *);
static errno_t ifproto_media_preout(struct ifnet *, protocol_family_t,
    mbuf_t *, const struct sockaddr *, void *, char *, char *);
static void ifproto_media_event(struct ifnet *, protocol_family_t,
    const struct kev_msg *);
static errno_t ifproto_media_ioctl(struct ifnet *, protocol_family_t,
    unsigned long, void *);
static errno_t ifproto_media_resolve_multi(ifnet_t, const struct sockaddr *,
    struct sockaddr_dl *, size_t);
static errno_t ifproto_media_send_arp(struct ifnet *, u_short,
    const struct sockaddr_dl *, const struct sockaddr *,
    const struct sockaddr_dl *, const struct sockaddr *);

static errno_t ifp_if_input(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, const struct ifnet_stat_increment_param *s,
    boolean_t poll, struct thread *tp);
static void ifp_if_input_poll(struct ifnet *, u_int32_t, u_int32_t,
    struct mbuf **, struct mbuf **, u_int32_t *, u_int32_t *);
static errno_t ifp_if_ctl(struct ifnet *, ifnet_ctl_cmd_t, u_int32_t, void *);
static errno_t ifp_if_demux(struct ifnet *, struct mbuf *, char *,
    protocol_family_t *);
static errno_t ifp_if_add_proto(struct ifnet *, protocol_family_t,
    const struct ifnet_demux_desc *, u_int32_t);
static errno_t ifp_if_del_proto(struct ifnet *, protocol_family_t);
static errno_t ifp_if_check_multi(struct ifnet *, const struct sockaddr *);
#if !XNU_TARGET_OS_OSX
static errno_t ifp_if_framer(struct ifnet *, struct mbuf **,
    const struct sockaddr *, const char *, const char *,
    u_int32_t *, u_int32_t *);
#else                        
static errno_t ifp_if_framer(struct ifnet *, struct mbuf **,
    const struct sockaddr *, const char *, const char *);
#endif                        
static errno_t ifp_if_framer_extended(struct ifnet *, struct mbuf **,
    const struct sockaddr *, const char *, const char *,
    u_int32_t *, u_int32_t *);
static errno_t ifp_if_set_bpf_tap(struct ifnet *, bpf_tap_mode, bpf_packet_func);
static void ifp_if_free(struct ifnet *);
static void ifp_if_event(struct ifnet *, const struct kev_msg *);
static __inline void ifp_inc_traffic_class_in(struct ifnet *, struct mbuf *);
static __inline void ifp_inc_traffic_class_out(struct ifnet *, struct mbuf *);

static errno_t dlil_input_async(struct dlil_threading_info *, struct ifnet *,
    struct mbuf *, struct mbuf *, const struct ifnet_stat_increment_param *,
    boolean_t, struct thread *);
static errno_t dlil_input_sync(struct dlil_threading_info *, struct ifnet *,
    struct mbuf *, struct mbuf *, const struct ifnet_stat_increment_param *,
    boolean_t, struct thread *);

static void dlil_main_input_thread_func(void *, wait_result_t);
static void dlil_main_input_thread_cont(void *, wait_result_t);

static void dlil_input_thread_func(void *, wait_result_t);
static void dlil_input_thread_cont(void *, wait_result_t);

static void dlil_rxpoll_input_thread_func(void *, wait_result_t);
static void dlil_rxpoll_input_thread_cont(void *, wait_result_t);

static int dlil_create_input_thread(ifnet_t, struct dlil_threading_info *,
    thread_continue_t *);
static void dlil_terminate_input_thread(struct dlil_threading_info *);
static void dlil_input_stats_add(const struct ifnet_stat_increment_param *,
    struct dlil_threading_info *, struct ifnet *, boolean_t);
static boolean_t dlil_input_stats_sync(struct ifnet *,
    struct dlil_threading_info *);
static void dlil_input_packet_list_common(struct ifnet *, struct mbuf *,
    u_int32_t, ifnet_model_t, boolean_t);
static errno_t ifnet_input_common(struct ifnet *, struct mbuf *, struct mbuf *,
    const struct ifnet_stat_increment_param *, boolean_t, boolean_t);
static int dlil_is_clat_needed(protocol_family_t, mbuf_t );
static errno_t dlil_clat46(ifnet_t, protocol_family_t *, mbuf_t *);
static errno_t dlil_clat64(ifnet_t, protocol_family_t *, mbuf_t *);
#if DEBUG || DEVELOPMENT
static void dlil_verify_sum16(void);
#endif                           
static void dlil_output_cksum_dbg(struct ifnet *, struct mbuf *, uint32_t,
    protocol_family_t);
static void dlil_input_cksum_dbg(struct ifnet *, struct mbuf *, char *,
    protocol_family_t);

static void dlil_incr_pending_thread_count(void);
static void dlil_decr_pending_thread_count(void);

static void ifnet_detacher_thread_func(void *, wait_result_t);
static void ifnet_detacher_thread_cont(void *, wait_result_t);
static void ifnet_detach_final(struct ifnet *);
static void ifnet_detaching_enqueue(struct ifnet *);
static struct ifnet *ifnet_detaching_dequeue(void);

static void ifnet_start_thread_func(void *, wait_result_t);
static void ifnet_start_thread_cont(void *, wait_result_t);

static void ifnet_poll_thread_func(void *, wait_result_t);
static void ifnet_poll_thread_cont(void *, wait_result_t);

static errno_t ifnet_enqueue_common(struct ifnet *, classq_pkt_t *,
    boolean_t, boolean_t *);

static void ifp_src_route_copyout(struct ifnet *, struct route *);
static void ifp_src_route_copyin(struct ifnet *, struct route *);
static void ifp_src_route6_copyout(struct ifnet *, struct route_in6 *);
static void ifp_src_route6_copyin(struct ifnet *, struct route_in6 *);

static int sysctl_rxpoll SYSCTL_HANDLER_ARGS;
static int sysctl_rxpoll_mode_holdtime SYSCTL_HANDLER_ARGS;
static int sysctl_rxpoll_sample_holdtime SYSCTL_HANDLER_ARGS;
static int sysctl_rxpoll_interval_time SYSCTL_HANDLER_ARGS;
static int sysctl_rxpoll_wlowat SYSCTL_HANDLER_ARGS;
static int sysctl_rxpoll_whiwat SYSCTL_HANDLER_ARGS;
static int sysctl_sndq_maxlen SYSCTL_HANDLER_ARGS;
static int sysctl_rcvq_maxlen SYSCTL_HANDLER_ARGS;
static int sysctl_hwcksum_dbg_mode SYSCTL_HANDLER_ARGS;
static int sysctl_hwcksum_dbg_partial_rxoff_forced SYSCTL_HANDLER_ARGS;
static int sysctl_hwcksum_dbg_partial_rxoff_adj SYSCTL_HANDLER_ARGS;

struct chain_len_stats tx_chain_len_stats;
static int sysctl_tx_chain_len_stats SYSCTL_HANDLER_ARGS;

#if TEST_INPUT_THREAD_TERMINATION
static int sysctl_input_thread_termination_spin SYSCTL_HANDLER_ARGS;
#endif                                    

                                                    
static TAILQ_HEAD(, ifnet) ifnet_detaching_head;
static u_int32_t ifnet_detaching_cnt;
static boolean_t ifnet_detaching_embryonic;
static void *ifnet_delayed_run;                                        

decl_lck_mtx_data(static, ifnet_fc_lock);

static uint32_t ifnet_flowhash_seed;

struct ifnet_flowhash_key {
	char            ifk_name[IFNAMSIZ];
	uint32_t        ifk_unit;
	uint32_t        ifk_flags;
	uint32_t        ifk_eflags;
	uint32_t        ifk_capabilities;
	uint32_t        ifk_capenable;
	uint32_t        ifk_output_sched_model;
	uint32_t        ifk_rand1;
	uint32_t        ifk_rand2;
};

                                      
struct ifnet_fc_entry {
	RB_ENTRY(ifnet_fc_entry) ifce_entry;
	u_int32_t       ifce_flowhash;
	struct ifnet    *ifce_ifp;
};

static uint32_t ifnet_calc_flowhash(struct ifnet *);
static int ifce_cmp(const struct ifnet_fc_entry *,
    const struct ifnet_fc_entry *);
static int ifnet_fc_add(struct ifnet *);
static struct ifnet_fc_entry *ifnet_fc_get(u_int32_t);
static void ifnet_fc_entry_free(struct ifnet_fc_entry *);

                                
RB_HEAD(ifnet_fc_tree, ifnet_fc_entry) ifnet_fc_tree;
RB_PROTOTYPE(ifnet_fc_tree, ifnet_fc_entry, ifce_entry, ifce_cmp);
RB_GENERATE(ifnet_fc_tree, ifnet_fc_entry, ifce_entry, ifce_cmp);

static ZONE_DECLARE(ifnet_fc_zone, "ifnet_fc_zone",
    sizeof(struct ifnet_fc_entry), ZC_ZFREE_CLEARMEM);

extern void bpfdetach(struct ifnet *);
extern void proto_input_run(void);

extern uint32_t udp_count_opportunistic(unsigned int ifindex,
    u_int32_t flags);
extern uint32_t tcp_count_opportunistic(unsigned int ifindex,
    u_int32_t flags);

__private_extern__ void link_rtrequest(int, struct rtentry *, struct sockaddr *);

#if CONFIG_MACF
#if !XNU_TARGET_OS_OSX
int dlil_lladdr_ckreq = 1;
#else                        
int dlil_lladdr_ckreq = 0;
#endif                        
#endif                  

#if DEBUG
int dlil_verbose = 1;
#else
int dlil_verbose = 0;
#endif            
#if IFNET_INPUT_SANITY_CHK
                                                    
static u_int32_t dlil_input_sanity_check = 0;
#endif                             
                               
struct timespec dlil_dbgrate = { .tv_sec = 1, .tv_nsec = 0 };

SYSCTL_DECL(_net_link_generic_system);

SYSCTL_INT(_net_link_generic_system, OID_AUTO, dlil_verbose,
    CTLFLAG_RW | CTLFLAG_LOCKED, &dlil_verbose, 0, "Log DLIL error messages");

#define IF_SNDQ_MINLEN  32
u_int32_t if_sndq_maxlen = IFQ_MAXLEN;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, sndq_maxlen,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &if_sndq_maxlen, IFQ_MAXLEN,
    sysctl_sndq_maxlen, "I", "Default transmit queue max length");

#define IF_RCVQ_MINLEN  32
#define IF_RCVQ_MAXLEN  256
u_int32_t if_rcvq_maxlen = IF_RCVQ_MAXLEN;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rcvq_maxlen,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &if_rcvq_maxlen, IFQ_MAXLEN,
    sysctl_rcvq_maxlen, "I", "Default receive queue max length");

#define IF_RXPOLL_DECAY         2                                         
u_int32_t if_rxpoll_decay = IF_RXPOLL_DECAY;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, rxpoll_decay,
    CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_decay, IF_RXPOLL_DECAY,
    "ilog2 of EWMA decay rate of avg inbound packets");

#define IF_RXPOLL_MODE_HOLDTIME_MIN     (10ULL * 1000 * 1000)              
#define IF_RXPOLL_MODE_HOLDTIME         (1000ULL * 1000 * 1000)            
static u_int64_t if_rxpoll_mode_holdtime = IF_RXPOLL_MODE_HOLDTIME;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll_freeze_time,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_mode_holdtime,
    IF_RXPOLL_MODE_HOLDTIME, sysctl_rxpoll_mode_holdtime,
    "Q", "input poll mode freeze time");

#define IF_RXPOLL_SAMPLETIME_MIN        (1ULL * 1000 * 1000)              
#define IF_RXPOLL_SAMPLETIME            (10ULL * 1000 * 1000)              
static u_int64_t if_rxpoll_sample_holdtime = IF_RXPOLL_SAMPLETIME;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll_sample_time,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_sample_holdtime,
    IF_RXPOLL_SAMPLETIME, sysctl_rxpoll_sample_holdtime,
    "Q", "input poll sampling time");

static u_int64_t if_rxpoll_interval_time = IF_RXPOLL_INTERVALTIME;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll_interval_time,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_interval_time,
    IF_RXPOLL_INTERVALTIME, sysctl_rxpoll_interval_time,
    "Q", "input poll interval (time)");

#define IF_RXPOLL_INTERVAL_PKTS 0                         
u_int32_t if_rxpoll_interval_pkts = IF_RXPOLL_INTERVAL_PKTS;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, rxpoll_interval_pkts,
    CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_interval_pkts,
    IF_RXPOLL_INTERVAL_PKTS, "input poll interval (packets)");

#define IF_RXPOLL_WLOWAT        10
static u_int32_t if_sysctl_rxpoll_wlowat = IF_RXPOLL_WLOWAT;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll_wakeups_lowat,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &if_sysctl_rxpoll_wlowat,
    IF_RXPOLL_WLOWAT, sysctl_rxpoll_wlowat,
    "I", "input poll wakeup low watermark");

#define IF_RXPOLL_WHIWAT        100
static u_int32_t if_sysctl_rxpoll_whiwat = IF_RXPOLL_WHIWAT;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll_wakeups_hiwat,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &if_sysctl_rxpoll_whiwat,
    IF_RXPOLL_WHIWAT, sysctl_rxpoll_whiwat,
    "I", "input poll wakeup high watermark");

static u_int32_t if_rxpoll_max = 0;                                        
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, rxpoll_max,
    CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll_max, 0,
    "max packets per poll call");

u_int32_t if_rxpoll = 1;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, rxpoll,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &if_rxpoll, 0,
    sysctl_rxpoll, "I", "enable opportunistic input polling");

#if TEST_INPUT_THREAD_TERMINATION
static u_int32_t if_input_thread_termination_spin = 0;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, input_thread_termination_spin,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &if_input_thread_termination_spin, 0,
    sysctl_input_thread_termination_spin,
    "I", "input thread termination spin limit");
#endif                                    

static u_int32_t cur_dlil_input_threads = 0;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, dlil_input_threads,
    CTLFLAG_RD | CTLFLAG_LOCKED, &cur_dlil_input_threads, 0,
    "Current number of DLIL input threads");

#if IFNET_INPUT_SANITY_CHK
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, dlil_input_sanity_check,
    CTLFLAG_RW | CTLFLAG_LOCKED, &dlil_input_sanity_check, 0,
    "Turn on sanity checking in DLIL input");
#endif                             

static u_int32_t if_flowadv = 1;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, flow_advisory,
    CTLFLAG_RW | CTLFLAG_LOCKED, &if_flowadv, 1,
    "enable flow-advisory mechanism");

static u_int32_t if_delaybased_queue = 1;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, delaybased_queue,
    CTLFLAG_RW | CTLFLAG_LOCKED, &if_delaybased_queue, 1,
    "enable delay based dynamic queue sizing");

static uint64_t hwcksum_in_invalidated = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_in_invalidated, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_in_invalidated, "inbound packets with invalidated hardware cksum");

uint32_t hwcksum_dbg = 0;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, hwcksum_dbg,
    CTLFLAG_RW | CTLFLAG_LOCKED, &hwcksum_dbg, 0,
    "enable hardware cksum debugging");

u_int32_t ifnet_start_delayed = 0;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, start_delayed,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ifnet_start_delayed, 0,
    "number of times start was delayed");

u_int32_t ifnet_delay_start_disabled = 0;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, start_delay_disabled,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ifnet_delay_start_disabled, 0,
    "number of times start was delayed");

static inline void
ifnet_delay_start_disabled_increment(void)
{
	OSIncrementAtomic(&ifnet_delay_start_disabled);
}

#define HWCKSUM_DBG_PARTIAL_FORCED      0x1                                  
#define HWCKSUM_DBG_PARTIAL_RXOFF_ADJ   0x2                              
#define HWCKSUM_DBG_FINALIZE_FORCED     0x10                         
#define HWCKSUM_DBG_MASK \
	(HWCKSUM_DBG_PARTIAL_FORCED | HWCKSUM_DBG_PARTIAL_RXOFF_ADJ |   \
	HWCKSUM_DBG_FINALIZE_FORCED)

static uint32_t hwcksum_dbg_mode = 0;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, hwcksum_dbg_mode,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &hwcksum_dbg_mode,
    0, sysctl_hwcksum_dbg_mode, "I", "hardware cksum debugging mode");

static uint64_t hwcksum_dbg_partial_forced = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_partial_forced, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_partial_forced, "packets forced using partial cksum");

static uint64_t hwcksum_dbg_partial_forced_bytes = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_partial_forced_bytes, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_partial_forced_bytes, "bytes forced using partial cksum");

static uint32_t hwcksum_dbg_partial_rxoff_forced = 0;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_partial_rxoff_forced, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &hwcksum_dbg_partial_rxoff_forced, 0,
    sysctl_hwcksum_dbg_partial_rxoff_forced, "I",
    "forced partial cksum rx offset");

static uint32_t hwcksum_dbg_partial_rxoff_adj = 0;
SYSCTL_PROC(_net_link_generic_system, OID_AUTO, hwcksum_dbg_partial_rxoff_adj,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &hwcksum_dbg_partial_rxoff_adj,
    0, sysctl_hwcksum_dbg_partial_rxoff_adj, "I",
    "adjusted partial cksum rx offset");

static uint64_t hwcksum_dbg_verified = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_verified, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_verified, "packets verified for having good checksum");

static uint64_t hwcksum_dbg_bad_cksum = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_bad_cksum, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_bad_cksum, "packets with bad hardware calculated checksum");

static uint64_t hwcksum_dbg_bad_rxoff = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_bad_rxoff, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_bad_rxoff, "packets with invalid rxoff");

static uint64_t hwcksum_dbg_adjusted = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_adjusted, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_adjusted, "packets with rxoff adjusted");

static uint64_t hwcksum_dbg_finalized_hdr = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_finalized_hdr, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_finalized_hdr, "finalized headers");

static uint64_t hwcksum_dbg_finalized_data = 0;
SYSCTL_QUAD(_net_link_generic_system, OID_AUTO,
    hwcksum_dbg_finalized_data, CTLFLAG_RD | CTLFLAG_LOCKED,
    &hwcksum_dbg_finalized_data, "finalized payloads");

uint32_t hwcksum_tx = 1;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, hwcksum_tx,
    CTLFLAG_RW | CTLFLAG_LOCKED, &hwcksum_tx, 0,
    "enable transmit hardware checksum offload");

uint32_t hwcksum_rx = 1;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, hwcksum_rx,
    CTLFLAG_RW | CTLFLAG_LOCKED, &hwcksum_rx, 0,
    "enable receive hardware checksum offload");

SYSCTL_PROC(_net_link_generic_system, OID_AUTO, tx_chain_len_stats,
    CTLFLAG_RD | CTLFLAG_LOCKED, 0, 9,
    sysctl_tx_chain_len_stats, "S", "");

uint32_t tx_chain_len_count = 0;
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, tx_chain_len_count,
    CTLFLAG_RW | CTLFLAG_LOCKED, &tx_chain_len_count, 0, "");

static uint32_t threshold_notify = 1;                               
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, threshold_notify,
    CTLFLAG_RW | CTLFLAG_LOCKED, &threshold_notify, 0, "");

static uint32_t threshold_interval = 2;                         
SYSCTL_UINT(_net_link_generic_system, OID_AUTO, threshold_interval,
    CTLFLAG_RW | CTLFLAG_LOCKED, &threshold_interval, 0, "");

#if (DEVELOPMENT || DEBUG)
static int sysctl_get_kao_frames SYSCTL_HANDLER_ARGS;
SYSCTL_NODE(_net_link_generic_system, OID_AUTO, get_kao_frames,
    CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_get_kao_frames, "");
#endif                           

struct net_api_stats net_api_stats;
SYSCTL_STRUCT(_net, OID_AUTO, api_stats, CTLFLAG_RD | CTLFLAG_LOCKED,
    &net_api_stats, net_api_stats, "");

unsigned int net_rxpoll = 1;
unsigned int net_affinity = 1;
unsigned int net_async = 1;                                          

static kern_return_t dlil_affinity_set(struct thread *, u_int32_t);

extern u_int32_t        inject_buckets;

static  lck_grp_attr_t  *dlil_grp_attributes = NULL;
static  lck_attr_t      *dlil_lck_attributes = NULL;

                                     
static void dlil_dt_tcall_fn(thread_call_param_t, thread_call_param_t);

void
ifnet_filter_update_tso(boolean_t filter_enable)
{
	  
                                                          
                                             
    
	OSAddAtomic(filter_enable ? 1 : -1, &dlil_filter_disable_tso_count);
	routegenid_update();
}


#define DLIL_INPUT_CHECK(m, ifp) {                                      \
	struct ifnet *_rcvif = mbuf_pkthdr_rcvif(m);                    \
	if (_rcvif == NULL || (ifp != lo_ifp && _rcvif != ifp) ||       \
	    !(mbuf_flags(m) & MBUF_PKTHDR)) {                           \
	        panic_plain("%s: invalid mbuf %p\n", __func__, m);      \
	                                                        \
	}                                                               \
}

#define DLIL_EWMA(old, new, decay) do {                                 \
	u_int32_t _avg;                                                 \
	if ((_avg = (old)) > 0)                                         \
	        _avg = (((_avg << (decay)) - _avg) + (new)) >> (decay); \
	else                                                            \
	        _avg = (new);                                           \
	(old) = _avg;                                                   \
} while (0)

#define MBPS    (1ULL * 1000 * 1000)
#define GBPS    (MBPS * 1000)

struct rxpoll_time_tbl {
	u_int64_t       speed;                              
	u_int32_t       plowat;                                    
	u_int32_t       phiwat;                                     
	u_int32_t       blowat;                                  
	u_int32_t       bhiwat;                                   
};

static struct rxpoll_time_tbl rxpoll_tbl[] = {
	{ .speed =  10 * MBPS, .plowat = 2, .phiwat = 8, .blowat = (1 * 1024), .bhiwat = (6 * 1024)    },
	{ .speed = 100 * MBPS, .plowat = 10, .phiwat = 40, .blowat = (4 * 1024), .bhiwat = (64 * 1024)   },
	{ .speed =   1 * GBPS, .plowat = 10, .phiwat = 40, .blowat = (4 * 1024), .bhiwat = (64 * 1024)   },
	{ .speed =  10 * GBPS, .plowat = 10, .phiwat = 40, .blowat = (4 * 1024), .bhiwat = (64 * 1024)   },
	{ .speed = 100 * GBPS, .plowat = 10, .phiwat = 40, .blowat = (4 * 1024), .bhiwat = (64 * 1024)   },
	{ .speed = 0, .plowat = 0, .phiwat = 0, .blowat = 0, .bhiwat = 0 }
};

decl_lck_mtx_data(static, dlil_thread_sync_lock);
static uint32_t dlil_pending_thread_cnt = 0;

static void
dlil_incr_pending_thread_count(void)
{
	LCK_MTX_ASSERT(&dlil_thread_sync_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(&dlil_thread_sync_lock);
	dlil_pending_thread_cnt++;
	lck_mtx_unlock(&dlil_thread_sync_lock);
}

static void
dlil_decr_pending_thread_count(void)
{
	LCK_MTX_ASSERT(&dlil_thread_sync_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(&dlil_thread_sync_lock);
	VERIFY(dlil_pending_thread_cnt > 0);
	dlil_pending_thread_cnt--;
	if (dlil_pending_thread_cnt == 0) {
		wakeup(&dlil_pending_thread_cnt);
	}
	lck_mtx_unlock(&dlil_thread_sync_lock);
}

int
proto_hash_value(u_int32_t protocol_family)
{
	  
                                                           
                                                         
                                                       
    
	switch (protocol_family) {
	case PF_INET:
		return 0;
	case PF_INET6:
		return 1;
	case PF_VLAN:
		return 2;
	case PF_802154:
		return 3;
	case PF_UNSPEC:
	default:
		return 4;
	}
}

  
                                             
   
static struct if_proto *
find_attached_proto(struct ifnet *ifp, u_int32_t protocol_family)
{
	struct if_proto *proto = NULL;
	u_int32_t i = proto_hash_value(protocol_family);

	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_OWNED);

	if (ifp->if_proto_hash != NULL) {
		proto = SLIST_FIRST(&ifp->if_proto_hash[i]);
	}

	while (proto != NULL && proto->protocol_family != protocol_family) {
		proto = SLIST_NEXT(proto, next_hash);
	}

	if (proto != NULL) {
		if_proto_ref(proto);
	}

	return proto;
}

static void
if_proto_ref(struct if_proto *proto)
{
	atomic_add_32(&proto->refcount, 1);
}

extern void if_rtproto_del(struct ifnet *ifp, int protocol);

static void
if_proto_free(struct if_proto *proto)
{
	u_int32_t oldval;
	struct ifnet *ifp = proto->ifp;
	u_int32_t proto_family = proto->protocol_family;
	struct kev_dl_proto_data ev_pr_data;

	oldval = atomic_add_32_ov(&proto->refcount, -1);
	if (oldval > 1) {
		return;
	}

	if (proto->proto_kpi == kProtoKPI_v1) {
		if (proto->kpi.v1.detached) {
			proto->kpi.v1.detached(ifp, proto->protocol_family);
		}
	}
	if (proto->proto_kpi == kProtoKPI_v2) {
		if (proto->kpi.v2.detached) {
			proto->kpi.v2.detached(ifp, proto->protocol_family);
		}
	}

	  
                                                                  
                            
    
	if_rtproto_del(ifp, proto_family);

	ifnet_lock_shared(ifp);

	                                                                 
	VERIFY(proto->detached);

	  
                                                                    
                       
    
	ev_pr_data.proto_family = proto_family;
	ev_pr_data.proto_remaining_count = dlil_ifp_protolist(ifp, NULL, 0);

	ifnet_lock_done(ifp);

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_PROTO_DETACHED,
	    (struct net_event_data *)&ev_pr_data,
	    sizeof(struct kev_dl_proto_data));

	if (ev_pr_data.proto_remaining_count == 0) {
		  
                                                                  
                                                                 
                                                            
     
		(void) ifnet_set_flags(ifp, 0, IFF_UP);
		(void) ifnet_ioctl(ifp, 0, SIOCSIFFLAGS, NULL);
		dlil_post_sifflags_msg(ifp);
	}

	zfree(dlif_proto_zone, proto);
}

__private_extern__ void
ifnet_lock_assert(struct ifnet *ifp, ifnet_lock_assert_t what)
{
#if !MACH_ASSERT
#pragma unused(ifp)
#endif
	unsigned int type = 0;
	int ass = 1;

	switch (what) {
	case IFNET_LCK_ASSERT_EXCLUSIVE:
		type = LCK_RW_ASSERT_EXCLUSIVE;
		break;

	case IFNET_LCK_ASSERT_SHARED:
		type = LCK_RW_ASSERT_SHARED;
		break;

	case IFNET_LCK_ASSERT_OWNED:
		type = LCK_RW_ASSERT_HELD;
		break;

	case IFNET_LCK_ASSERT_NOTOWNED:
		                                                   
		ass = 0;
		break;

	default:
		panic("bad ifnet assert type: %d", what);
		                
	}
	if (ass) {
		LCK_RW_ASSERT(&ifp->if_lock, type);
	}
}

__private_extern__ void
ifnet_lock_shared(struct ifnet *ifp)
{
	lck_rw_lock_shared(&ifp->if_lock);
}

__private_extern__ void
ifnet_lock_exclusive(struct ifnet *ifp)
{
	lck_rw_lock_exclusive(&ifp->if_lock);
}

__private_extern__ void
ifnet_lock_done(struct ifnet *ifp)
{
	lck_rw_done(&ifp->if_lock);
}

#if INET
__private_extern__ void
if_inetdata_lock_shared(struct ifnet *ifp)
{
	lck_rw_lock_shared(&ifp->if_inetdata_lock);
}

__private_extern__ void
if_inetdata_lock_exclusive(struct ifnet *ifp)
{
	lck_rw_lock_exclusive(&ifp->if_inetdata_lock);
}

__private_extern__ void
if_inetdata_lock_done(struct ifnet *ifp)
{
	lck_rw_done(&ifp->if_inetdata_lock);
}
#endif

__private_extern__ void
if_inet6data_lock_shared(struct ifnet *ifp)
{
	lck_rw_lock_shared(&ifp->if_inet6data_lock);
}

__private_extern__ void
if_inet6data_lock_exclusive(struct ifnet *ifp)
{
	lck_rw_lock_exclusive(&ifp->if_inet6data_lock);
}

__private_extern__ void
if_inet6data_lock_done(struct ifnet *ifp)
{
	lck_rw_done(&ifp->if_inet6data_lock);
}

__private_extern__ void
ifnet_head_lock_shared(void)
{
	lck_rw_lock_shared(&ifnet_head_lock);
}

__private_extern__ void
ifnet_head_lock_exclusive(void)
{
	lck_rw_lock_exclusive(&ifnet_head_lock);
}

__private_extern__ void
ifnet_head_done(void)
{
	lck_rw_done(&ifnet_head_lock);
}

__private_extern__ void
ifnet_head_assert_exclusive(void)
{
	LCK_RW_ASSERT(&ifnet_head_lock, LCK_RW_ASSERT_EXCLUSIVE);
}

  
                     
                                                                            
                          
                                                                             
  
        
                                               
   
static u_int32_t
dlil_ifp_protolist(struct ifnet *ifp, protocol_family_t *list,
    u_int32_t list_count)
{
	u_int32_t       count = 0;
	int             i;

	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_OWNED);

	if (ifp->if_proto_hash == NULL) {
		goto done;
	}

	for (i = 0; i < PROTO_HASH_SLOTS; i++) {
		struct if_proto *proto;
		SLIST_FOREACH(proto, &ifp->if_proto_hash[i], next_hash) {
			if (list != NULL && count < list_count) {
				list[count] = proto->protocol_family;
			}
			count++;
		}
	}
done:
	return count;
}

__private_extern__ u_int32_t
if_get_protolist(struct ifnet * ifp, u_int32_t *protolist, u_int32_t count)
{
	ifnet_lock_shared(ifp);
	count = dlil_ifp_protolist(ifp, protolist, count);
	ifnet_lock_done(ifp);
	return count;
}

__private_extern__ void
if_free_protolist(u_int32_t *list)
{
	_FREE(list, M_TEMP);
}

__private_extern__ int
dlil_post_msg(struct ifnet *ifp, u_int32_t event_subclass,
    u_int32_t event_code, struct net_event_data *event_data,
    u_int32_t event_data_len)
{
	struct net_event_data ev_data;
	struct kev_msg ev_msg;

	bzero(&ev_msg, sizeof(ev_msg));
	bzero(&ev_data, sizeof(ev_data));
	  
                                                             
                                                     
                                            
    
	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     = event_subclass;
	ev_msg.event_code       = event_code;

	if (event_data == NULL) {
		event_data = &ev_data;
		event_data_len = sizeof(struct net_event_data);
	}

	strlcpy(&event_data->if_name[0], ifp->if_name, IFNAMSIZ);
	event_data->if_family = ifp->if_family;
	event_data->if_unit   = (u_int32_t)ifp->if_unit;

	ev_msg.dv[0].data_length = event_data_len;
	ev_msg.dv[0].data_ptr    = event_data;
	ev_msg.dv[1].data_length = 0;

	bool update_generation = true;
	if (event_subclass == KEV_DL_SUBCLASS) {
		                                                                                    
		switch (event_code) {
		case KEV_DL_LINK_QUALITY_METRIC_CHANGED:
		case KEV_DL_RRC_STATE_CHANGED:
		case KEV_DL_NODE_PRESENCE:
		case KEV_DL_NODE_ABSENCE:
		case KEV_DL_MASTER_ELECTED:
			update_generation = false;
			break;
		default:
			break;
		}
	}

	return dlil_event_internal(ifp, &ev_msg, update_generation);
}

__private_extern__ int
dlil_alloc_local_stats(struct ifnet *ifp)
{
	int ret = EINVAL;
	void *buf, *base, **pbuf;

	if (ifp == NULL) {
		goto end;
	}

	if (ifp->if_tcp_stat == NULL && ifp->if_udp_stat == NULL) {
		                                      
		buf = zalloc_flags(dlif_tcpstat_zone, Z_WAITOK | Z_ZERO);
		if (buf == NULL) {
			ret = ENOMEM;
			goto end;
		}

		                                                         
		base = (void *)P2ROUNDUP((intptr_t)buf + sizeof(u_int64_t),
		    sizeof(u_int64_t));
		VERIFY(((intptr_t)base + dlif_tcpstat_size) <=
		    ((intptr_t)buf + dlif_tcpstat_bufsize));

		  
                                                       
                                                       
     
		pbuf = (void **)((intptr_t)base - sizeof(void *));
		*pbuf = buf;
		ifp->if_tcp_stat = base;

		                                      
		buf = zalloc_flags(dlif_udpstat_zone, Z_WAITOK | Z_ZERO);
		if (buf == NULL) {
			ret = ENOMEM;
			goto end;
		}

		                                                         
		base = (void *)P2ROUNDUP((intptr_t)buf + sizeof(u_int64_t),
		    sizeof(u_int64_t));
		VERIFY(((intptr_t)base + dlif_udpstat_size) <=
		    ((intptr_t)buf + dlif_udpstat_bufsize));

		  
                                                       
                                                       
     
		pbuf = (void **)((intptr_t)base - sizeof(void *));
		*pbuf = buf;
		ifp->if_udp_stat = base;

		VERIFY(IS_P2ALIGNED(ifp->if_tcp_stat, sizeof(u_int64_t)) &&
		    IS_P2ALIGNED(ifp->if_udp_stat, sizeof(u_int64_t)));

		ret = 0;
	}

	if (ifp->if_ipv4_stat == NULL) {
		MALLOC(ifp->if_ipv4_stat, struct if_tcp_ecn_stat *,
		    sizeof(struct if_tcp_ecn_stat), M_TEMP, M_WAITOK | M_ZERO);
		if (ifp->if_ipv4_stat == NULL) {
			ret = ENOMEM;
			goto end;
		}
	}

	if (ifp->if_ipv6_stat == NULL) {
		MALLOC(ifp->if_ipv6_stat, struct if_tcp_ecn_stat *,
		    sizeof(struct if_tcp_ecn_stat), M_TEMP, M_WAITOK | M_ZERO);
		if (ifp->if_ipv6_stat == NULL) {
			ret = ENOMEM;
			goto end;
		}
	}
end:
	if (ifp != NULL && ret != 0) {
		if (ifp->if_tcp_stat != NULL) {
			pbuf = (void **)
			    ((intptr_t)ifp->if_tcp_stat - sizeof(void *));
			zfree(dlif_tcpstat_zone, *pbuf);
			ifp->if_tcp_stat = NULL;
		}
		if (ifp->if_udp_stat != NULL) {
			pbuf = (void **)
			    ((intptr_t)ifp->if_udp_stat - sizeof(void *));
			zfree(dlif_udpstat_zone, *pbuf);
			ifp->if_udp_stat = NULL;
		}
		if (ifp->if_ipv4_stat != NULL) {
			FREE(ifp->if_ipv4_stat, M_TEMP);
			ifp->if_ipv4_stat = NULL;
		}
		if (ifp->if_ipv6_stat != NULL) {
			FREE(ifp->if_ipv6_stat, M_TEMP);
			ifp->if_ipv6_stat = NULL;
		}
	}

	return ret;
}

static void
dlil_reset_rxpoll_params(ifnet_t ifp)
{
	ASSERT(ifp != NULL);
	ifnet_set_poll_cycle(ifp, NULL);
	ifp->if_poll_update = 0;
	ifp->if_poll_flags = 0;
	ifp->if_poll_req = 0;
	ifp->if_poll_mode = IFNET_MODEL_INPUT_POLL_OFF;
	bzero(&ifp->if_poll_tstats, sizeof(ifp->if_poll_tstats));
	bzero(&ifp->if_poll_pstats, sizeof(ifp->if_poll_pstats));
	bzero(&ifp->if_poll_sstats, sizeof(ifp->if_poll_sstats));
	net_timerclear(&ifp->if_poll_mode_holdtime);
	net_timerclear(&ifp->if_poll_mode_lasttime);
	net_timerclear(&ifp->if_poll_sample_holdtime);
	net_timerclear(&ifp->if_poll_sample_lasttime);
	net_timerclear(&ifp->if_poll_dbg_lasttime);
}

static int
dlil_create_input_thread(ifnet_t ifp, struct dlil_threading_info *inp,
    thread_continue_t *thfunc)
{
	boolean_t dlil_rxpoll_input;
	thread_continue_t func = NULL;
	u_int32_t limit;
	int error = 0;

	dlil_rxpoll_input = (ifp != NULL && net_rxpoll &&
	    (ifp->if_eflags & IFEF_RXPOLL) && (ifp->if_xflags & IFXF_LEGACY));

	                                                      
	inp->dlth_strategy = dlil_input_async;

	                                                                        
	if (ifp == NULL) {
		  
                            
     
		func = dlil_main_input_thread_func;
		VERIFY(inp == dlil_main_input_thread);
		(void) strlcat(inp->dlth_name,
		    "main_input", DLIL_THREADNAME_LEN);
	} else if (dlil_rxpoll_input) {
		  
                                       
     
		func = dlil_rxpoll_input_thread_func;
		VERIFY(inp != dlil_main_input_thread);
		(void) snprintf(inp->dlth_name, DLIL_THREADNAME_LEN,
		    "%s_input_poll", if_name(ifp));
	} else if (net_async || (ifp->if_xflags & IFXF_LEGACY)) {
		  
                           
     
		func = dlil_input_thread_func;
		VERIFY(inp != dlil_main_input_thread);
		(void) snprintf(inp->dlth_name, DLIL_THREADNAME_LEN,
		    "%s_input", if_name(ifp));
	} else {
		  
                                                      
                                                
     
		ASSERT(func == NULL);
		ASSERT(!(ifp->if_xflags & IFXF_LEGACY));
		VERIFY(inp != dlil_main_input_thread);
		ASSERT(!inp->dlth_affinity);
		inp->dlth_strategy = dlil_input_sync;
	}
	VERIFY(inp->dlth_thread == THREAD_NULL);

	                     
	if (thfunc != NULL) {
		*thfunc = func;
	}

	inp->dlth_lock_grp = lck_grp_alloc_init(inp->dlth_name,
	    dlil_grp_attributes);
	lck_mtx_init(&inp->dlth_lock, inp->dlth_lock_grp, dlil_lck_attributes);

	inp->dlth_ifp = ifp;                                 
	  
                                                              
                                                                  
                                                            
                             
    
	if (ifp != NULL && net_rxpoll && (ifp->if_eflags & IFEF_RXPOLL)) {
		limit = MAX(if_rcvq_maxlen, IF_RCVQ_MINLEN);
		if (ifp->if_xflags & IFXF_LEGACY) {
			(void) dlil_rxpoll_set_params(ifp, NULL, FALSE);
		}
	} else {
		limit = (u_int32_t)-1;
	}

	_qinit(&inp->dlth_pkts, Q_DROPTAIL, limit, QP_MBUF);
	if (inp == dlil_main_input_thread) {
		struct dlil_main_threading_info *inpm =
		    (struct dlil_main_threading_info *)inp;
		_qinit(&inpm->lo_rcvq_pkts, Q_DROPTAIL, limit, QP_MBUF);
	}

	if (func == NULL) {
		ASSERT(!(ifp->if_xflags & IFXF_LEGACY));
		ASSERT(error == 0);
		error = ENODEV;
		goto done;
	}

	error = kernel_thread_start(func, inp, &inp->dlth_thread);
	if (error == KERN_SUCCESS) {
		thread_precedence_policy_data_t info;
		__unused kern_return_t kret;

		bzero(&info, sizeof(info));
		info.importance = 0;
		kret = thread_policy_set(inp->dlth_thread,
		    THREAD_PRECEDENCE_POLICY, (thread_policy_t)&info,
		    THREAD_PRECEDENCE_POLICY_COUNT);
		ASSERT(kret == KERN_SUCCESS);
		  
                                                            
                                                       
                                                             
     
		if (net_affinity) {
			struct thread *tp = inp->dlth_thread;
			u_int32_t tag;
			  
                                         
                                          
      
			read_frandom(&tag, sizeof(tag));
			if (dlil_affinity_set(tp, tag) == KERN_SUCCESS) {
				thread_reference(tp);
				inp->dlth_affinity_tag = tag;
				inp->dlth_affinity = TRUE;
			}
		}
	} else if (inp == dlil_main_input_thread) {
		panic_plain("%s: couldn't create main input thread", __func__);
		                
	} else {
		panic_plain("%s: couldn't create %s input thread", __func__,
		    if_name(ifp));
		                
	}
	OSAddAtomic(1, &cur_dlil_input_threads);

done:
	return error;
}

#if TEST_INPUT_THREAD_TERMINATION
static int
sysctl_input_thread_termination_spin SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint32_t i;
	int err;

	i = if_input_thread_termination_spin;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (net_rxpoll == 0) {
		return ENXIO;
	}

	if_input_thread_termination_spin = i;
	return err;
}
#endif                                    

static void
dlil_clean_threading_info(struct dlil_threading_info *inp)
{
	lck_mtx_destroy(&inp->dlth_lock, inp->dlth_lock_grp);
	lck_grp_free(inp->dlth_lock_grp);
	inp->dlth_lock_grp = NULL;

	inp->dlth_flags = 0;
	inp->dlth_wtot = 0;
	bzero(inp->dlth_name, sizeof(inp->dlth_name));
	inp->dlth_ifp = NULL;
	VERIFY(qhead(&inp->dlth_pkts) == NULL && qempty(&inp->dlth_pkts));
	qlimit(&inp->dlth_pkts) = 0;
	bzero(&inp->dlth_stats, sizeof(inp->dlth_stats));

	VERIFY(!inp->dlth_affinity);
	inp->dlth_thread = THREAD_NULL;
	inp->dlth_strategy = NULL;
	VERIFY(inp->dlth_driver_thread == THREAD_NULL);
	VERIFY(inp->dlth_poller_thread == THREAD_NULL);
	VERIFY(inp->dlth_affinity_tag == 0);
#if IFNET_INPUT_SANITY_CHK
	inp->dlth_pkts_cnt = 0;
#endif                             
}

static void
dlil_terminate_input_thread(struct dlil_threading_info *inp)
{
	struct ifnet *ifp = inp->dlth_ifp;
	classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);

	VERIFY(current_thread() == inp->dlth_thread);
	VERIFY(inp != dlil_main_input_thread);

	OSAddAtomic(-1, &cur_dlil_input_threads);

#if TEST_INPUT_THREAD_TERMINATION
	{                                                         
		uint32_t        v = 1;
		for (uint32_t i = 0;
		    i < if_input_thread_termination_spin;
		    i++) {
			v = (i + 1) * v;
		}
		DLIL_PRINTF("the value is %d\n", v);
	}
#endif                                    

	lck_mtx_lock_spin(&inp->dlth_lock);
	_getq_all(&inp->dlth_pkts, &pkt, NULL, NULL, NULL);
	VERIFY((inp->dlth_flags & DLIL_INPUT_TERMINATE) != 0);
	inp->dlth_flags |= DLIL_INPUT_TERMINATE_COMPLETE;
	wakeup_one((caddr_t)&inp->dlth_flags);
	lck_mtx_unlock(&inp->dlth_lock);

	                             
	if (pkt.cp_mbuf != NULL) {
		mbuf_freem_list(pkt.cp_mbuf);
	}

	                                                     
	thread_deallocate(current_thread());

	if (dlil_verbose) {
		DLIL_PRINTF("%s: input thread terminated\n",
		    if_name(ifp));
	}

	                     
	thread_terminate(current_thread());
	                
}

static kern_return_t
dlil_affinity_set(struct thread *tp, u_int32_t tag)
{
	thread_affinity_policy_data_t policy;

	bzero(&policy, sizeof(policy));
	policy.affinity_tag = tag;
	return thread_policy_set(tp, THREAD_AFFINITY_POLICY,
	           (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
}

void
dlil_init(void)
{
	thread_t thread = THREAD_NULL;

	  
                                                                      
    
	IF_DATA_REQUIRE_ALIGNED_64(ifi_ipackets);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_ierrors);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_opackets);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_oerrors);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_collisions);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_ibytes);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_obytes);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_imcasts);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_omcasts);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_iqdrops);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_noproto);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_alignerrs);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_dt_bytes);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_fpackets);
	IF_DATA_REQUIRE_ALIGNED_64(ifi_fbytes);

	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_ipackets);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_ierrors);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_opackets);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_oerrors);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_collisions);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_ibytes);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_obytes);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_imcasts);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_omcasts);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_iqdrops);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_noproto);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_alignerrs);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_dt_bytes);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_fpackets);
	IFNET_IF_DATA_REQUIRE_ALIGNED_64(ifi_fbytes);

	  
                                                                         
    
	_CASSERT(IF_HWASSIST_CSUM_IP == IFNET_CSUM_IP);
	_CASSERT(IF_HWASSIST_CSUM_TCP == IFNET_CSUM_TCP);
	_CASSERT(IF_HWASSIST_CSUM_UDP == IFNET_CSUM_UDP);
	_CASSERT(IF_HWASSIST_CSUM_IP_FRAGS == IFNET_CSUM_FRAGMENT);
	_CASSERT(IF_HWASSIST_CSUM_FRAGMENT == IFNET_IP_FRAGMENT);
	_CASSERT(IF_HWASSIST_CSUM_TCPIPV6 == IFNET_CSUM_TCPIPV6);
	_CASSERT(IF_HWASSIST_CSUM_UDPIPV6 == IFNET_CSUM_UDPIPV6);
	_CASSERT(IF_HWASSIST_CSUM_FRAGMENT_IPV6 == IFNET_IPV6_FRAGMENT);
	_CASSERT(IF_HWASSIST_CSUM_PARTIAL == IFNET_CSUM_PARTIAL);
	_CASSERT(IF_HWASSIST_CSUM_ZERO_INVERT == IFNET_CSUM_ZERO_INVERT);
	_CASSERT(IF_HWASSIST_VLAN_TAGGING == IFNET_VLAN_TAGGING);
	_CASSERT(IF_HWASSIST_VLAN_MTU == IFNET_VLAN_MTU);
	_CASSERT(IF_HWASSIST_TSO_V4 == IFNET_TSO_IPV4);
	_CASSERT(IF_HWASSIST_TSO_V6 == IFNET_TSO_IPV6);

	  
                                                        
    
	_CASSERT(CSUM_IP == IF_HWASSIST_CSUM_IP);
	_CASSERT(CSUM_TCP == IF_HWASSIST_CSUM_TCP);
	_CASSERT(CSUM_UDP == IF_HWASSIST_CSUM_UDP);
	_CASSERT(CSUM_IP_FRAGS == IF_HWASSIST_CSUM_IP_FRAGS);
	_CASSERT(CSUM_FRAGMENT == IF_HWASSIST_CSUM_FRAGMENT);
	_CASSERT(CSUM_TCPIPV6 == IF_HWASSIST_CSUM_TCPIPV6);
	_CASSERT(CSUM_UDPIPV6 == IF_HWASSIST_CSUM_UDPIPV6);
	_CASSERT(CSUM_FRAGMENT_IPV6 == IF_HWASSIST_CSUM_FRAGMENT_IPV6);
	_CASSERT(CSUM_PARTIAL == IF_HWASSIST_CSUM_PARTIAL);
	_CASSERT(CSUM_ZERO_INVERT == IF_HWASSIST_CSUM_ZERO_INVERT);
	_CASSERT(CSUM_VLAN_TAG_VALID == IF_HWASSIST_VLAN_TAGGING);

	  
                                                                     
    
	_CASSERT(IF_LLREACH_MAXLEN <= IF_LLREACHINFO_ADDRLEN);
	_CASSERT(IFNET_LLREACHINFO_ADDRLEN == IF_LLREACHINFO_ADDRLEN);

	_CASSERT(IFRLOGF_DLIL == IFNET_LOGF_DLIL);
	_CASSERT(IFRLOGF_FAMILY == IFNET_LOGF_FAMILY);
	_CASSERT(IFRLOGF_DRIVER == IFNET_LOGF_DRIVER);
	_CASSERT(IFRLOGF_FIRMWARE == IFNET_LOGF_FIRMWARE);

	_CASSERT(IFRLOGCAT_CONNECTIVITY == IFNET_LOGCAT_CONNECTIVITY);
	_CASSERT(IFRLOGCAT_QUALITY == IFNET_LOGCAT_QUALITY);
	_CASSERT(IFRLOGCAT_PERFORMANCE == IFNET_LOGCAT_PERFORMANCE);

	_CASSERT(IFRTYPE_FAMILY_ANY == IFNET_FAMILY_ANY);
	_CASSERT(IFRTYPE_FAMILY_LOOPBACK == IFNET_FAMILY_LOOPBACK);
	_CASSERT(IFRTYPE_FAMILY_ETHERNET == IFNET_FAMILY_ETHERNET);
	_CASSERT(IFRTYPE_FAMILY_SLIP == IFNET_FAMILY_SLIP);
	_CASSERT(IFRTYPE_FAMILY_TUN == IFNET_FAMILY_TUN);
	_CASSERT(IFRTYPE_FAMILY_VLAN == IFNET_FAMILY_VLAN);
	_CASSERT(IFRTYPE_FAMILY_PPP == IFNET_FAMILY_PPP);
	_CASSERT(IFRTYPE_FAMILY_PVC == IFNET_FAMILY_PVC);
	_CASSERT(IFRTYPE_FAMILY_DISC == IFNET_FAMILY_DISC);
	_CASSERT(IFRTYPE_FAMILY_MDECAP == IFNET_FAMILY_MDECAP);
	_CASSERT(IFRTYPE_FAMILY_GIF == IFNET_FAMILY_GIF);
	_CASSERT(IFRTYPE_FAMILY_FAITH == IFNET_FAMILY_FAITH);
	_CASSERT(IFRTYPE_FAMILY_STF == IFNET_FAMILY_STF);
	_CASSERT(IFRTYPE_FAMILY_FIREWIRE == IFNET_FAMILY_FIREWIRE);
	_CASSERT(IFRTYPE_FAMILY_BOND == IFNET_FAMILY_BOND);
	_CASSERT(IFRTYPE_FAMILY_CELLULAR == IFNET_FAMILY_CELLULAR);
	_CASSERT(IFRTYPE_FAMILY_6LOWPAN == IFNET_FAMILY_6LOWPAN);
	_CASSERT(IFRTYPE_FAMILY_UTUN == IFNET_FAMILY_UTUN);
	_CASSERT(IFRTYPE_FAMILY_IPSEC == IFNET_FAMILY_IPSEC);

	_CASSERT(IFRTYPE_SUBFAMILY_ANY == IFNET_SUBFAMILY_ANY);
	_CASSERT(IFRTYPE_SUBFAMILY_USB == IFNET_SUBFAMILY_USB);
	_CASSERT(IFRTYPE_SUBFAMILY_BLUETOOTH == IFNET_SUBFAMILY_BLUETOOTH);
	_CASSERT(IFRTYPE_SUBFAMILY_WIFI == IFNET_SUBFAMILY_WIFI);
	_CASSERT(IFRTYPE_SUBFAMILY_THUNDERBOLT == IFNET_SUBFAMILY_THUNDERBOLT);
	_CASSERT(IFRTYPE_SUBFAMILY_RESERVED == IFNET_SUBFAMILY_RESERVED);
	_CASSERT(IFRTYPE_SUBFAMILY_INTCOPROC == IFNET_SUBFAMILY_INTCOPROC);
	_CASSERT(IFRTYPE_SUBFAMILY_QUICKRELAY == IFNET_SUBFAMILY_QUICKRELAY);
	_CASSERT(IFRTYPE_SUBFAMILY_DEFAULT == IFNET_SUBFAMILY_DEFAULT);

	_CASSERT(DLIL_MODIDLEN == IFNET_MODIDLEN);
	_CASSERT(DLIL_MODARGLEN == IFNET_MODARGLEN);

	PE_parse_boot_argn("net_affinity", &net_affinity,
	    sizeof(net_affinity));

	PE_parse_boot_argn("net_rxpoll", &net_rxpoll, sizeof(net_rxpoll));

	PE_parse_boot_argn("net_rtref", &net_rtref, sizeof(net_rtref));

	PE_parse_boot_argn("net_async", &net_async, sizeof(net_async));

	PE_parse_boot_argn("ifnet_debug", &ifnet_debug, sizeof(ifnet_debug));

	VERIFY(dlil_pending_thread_cnt == 0);
	dlif_size = (ifnet_debug == 0) ? sizeof(struct dlil_ifnet) :
	    sizeof(struct dlil_ifnet_dbg);
	                                                       
	dlif_bufsize = dlif_size + sizeof(void *) + sizeof(u_int64_t);
	dlif_bufsize = (uint32_t)P2ROUNDUP(dlif_bufsize, sizeof(u_int64_t));
	dlif_zone = zone_create(DLIF_ZONE_NAME, dlif_bufsize, ZC_ZFREE_CLEARMEM);

	dlif_tcpstat_size = sizeof(struct tcpstat_local);
	                                                          
	dlif_tcpstat_bufsize =
	    dlif_tcpstat_size + sizeof(void *) + sizeof(u_int64_t);
	dlif_tcpstat_bufsize = (uint32_t)
	    P2ROUNDUP(dlif_tcpstat_bufsize, sizeof(u_int64_t));
	dlif_tcpstat_zone = zone_create(DLIF_TCPSTAT_ZONE_NAME,
	    dlif_tcpstat_bufsize, ZC_ZFREE_CLEARMEM);

	dlif_udpstat_size = sizeof(struct udpstat_local);
	                                                          
	dlif_udpstat_bufsize =
	    dlif_udpstat_size + sizeof(void *) + sizeof(u_int64_t);
	dlif_udpstat_bufsize = (uint32_t)
	    P2ROUNDUP(dlif_udpstat_bufsize, sizeof(u_int64_t));
	dlif_udpstat_zone = zone_create(DLIF_UDPSTAT_ZONE_NAME,
	    dlif_udpstat_bufsize, ZC_ZFREE_CLEARMEM);

	eventhandler_lists_ctxt_init(&ifnet_evhdlr_ctxt);

	TAILQ_INIT(&dlil_ifnet_head);
	TAILQ_INIT(&ifnet_head);
	TAILQ_INIT(&ifnet_detaching_head);
	TAILQ_INIT(&ifnet_ordered_head);

	                                       
	dlil_grp_attributes = lck_grp_attr_alloc_init();

	dlil_lock_group = lck_grp_alloc_init("DLIL internal locks",
	    dlil_grp_attributes);
	ifnet_lock_group = lck_grp_alloc_init("ifnet locks",
	    dlil_grp_attributes);
	ifnet_head_lock_group = lck_grp_alloc_init("ifnet head lock",
	    dlil_grp_attributes);
	ifnet_rcv_lock_group = lck_grp_alloc_init("ifnet rcv locks",
	    dlil_grp_attributes);
	ifnet_snd_lock_group = lck_grp_alloc_init("ifnet snd locks",
	    dlil_grp_attributes);

	                                           
	dlil_lck_attributes = lck_attr_alloc_init();

	ifnet_lock_attr = lck_attr_alloc_init();

	lck_rw_init(&ifnet_head_lock, ifnet_head_lock_group,
	    dlil_lck_attributes);
	lck_mtx_init(&dlil_ifnet_lock, dlil_lock_group, dlil_lck_attributes);
	lck_mtx_init(&dlil_thread_sync_lock, dlil_lock_group, dlil_lck_attributes);

	                                                
	lck_mtx_init(&ifnet_fc_lock, dlil_lock_group, dlil_lck_attributes);

	                                            
	ifa_init();

#if PF
	                                  
	pfinit();
#endif         

	                                 
	classq_init();

	                                  
	pktsched_init();

	                                        
	flowadv_init();

	                                            
	pktap_init();

	                                              
	net_qos_map_init();

	                                        
	if_ports_used_init();

	                                                           
	if_low_power_evhdlr_init();

#if DEBUG || DEVELOPMENT
	                    
	dlil_verify_sum16();
#endif                           

	                                 
	lltable_glbl_init();

	  
                                                                    
                                                    
    
	dlil_incr_pending_thread_count();
	(void) dlil_create_input_thread(NULL, dlil_main_input_thread, NULL);

	  
                                 
                                                                  
                                                             
                                                          
                        
    
	dlil_incr_pending_thread_count();
	if (kernel_thread_start(ifnet_detacher_thread_func,
	    NULL, &thread) != KERN_SUCCESS) {
		panic_plain("%s: couldn't create detacher thread", __func__);
		                
	}
	thread_deallocate(thread);

	  
                                                       
                                                     
    
	lck_mtx_lock(&dlil_thread_sync_lock);
	while (dlil_pending_thread_cnt != 0) {
		DLIL_PRINTF("%s: Waiting for all the create dlil kernel "
		    "threads to get scheduled at least once.\n", __func__);
		(void) msleep(&dlil_pending_thread_cnt, &dlil_thread_sync_lock,
		    (PZERO - 1), __func__, NULL);
		LCK_MTX_ASSERT(&dlil_thread_sync_lock, LCK_ASSERT_OWNED);
	}
	lck_mtx_unlock(&dlil_thread_sync_lock);
	DLIL_PRINTF("%s: All the created dlil kernel threads have been "
	    "scheduled at least once. Proceeding.\n", __func__);
}

static void
if_flt_monitor_busy(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(&ifp->if_flt_lock, LCK_MTX_ASSERT_OWNED);

	++ifp->if_flt_busy;
	VERIFY(ifp->if_flt_busy != 0);
}

static void
if_flt_monitor_unbusy(struct ifnet *ifp)
{
	if_flt_monitor_leave(ifp);
}

static void
if_flt_monitor_enter(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(&ifp->if_flt_lock, LCK_MTX_ASSERT_OWNED);

	while (ifp->if_flt_busy) {
		++ifp->if_flt_waiters;
		(void) msleep(&ifp->if_flt_head, &ifp->if_flt_lock,
		    (PZERO - 1), "if_flt_monitor", NULL);
	}
	if_flt_monitor_busy(ifp);
}

static void
if_flt_monitor_leave(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(&ifp->if_flt_lock, LCK_MTX_ASSERT_OWNED);

	VERIFY(ifp->if_flt_busy != 0);
	--ifp->if_flt_busy;

	if (ifp->if_flt_busy == 0 && ifp->if_flt_waiters > 0) {
		ifp->if_flt_waiters = 0;
		wakeup(&ifp->if_flt_head);
	}
}

__private_extern__ int
dlil_attach_filter(struct ifnet *ifp, const struct iff_filter *if_filter,
    interface_filter_t *filter_ref, u_int32_t flags)
{
	int retval = 0;
	struct ifnet_filter *filter = NULL;

	ifnet_head_lock_shared();
	                                                    
	if (!ifnet_lookup(ifp)) {
		retval = ENXIO;
		goto done;
	}

	filter = zalloc_flags(dlif_filt_zone, Z_WAITOK | Z_ZERO);
	if (filter == NULL) {
		retval = ENOMEM;
		goto done;
	}

	                                     
	filter->filt_flags = flags;
	filter->filt_ifp = ifp;
	filter->filt_cookie = if_filter->iff_cookie;
	filter->filt_name = if_filter->iff_name;
	filter->filt_protocol = if_filter->iff_protocol;
	  
                                                                 
    
	if (!IFNET_IS_INTCOPROC(ifp)) {
		filter->filt_input = if_filter->iff_input;
		filter->filt_output = if_filter->iff_output;
		filter->filt_event = if_filter->iff_event;
		filter->filt_ioctl = if_filter->iff_ioctl;
	}
	filter->filt_detached = if_filter->iff_detached;

	lck_mtx_lock(&ifp->if_flt_lock);
	if_flt_monitor_enter(ifp);

	LCK_MTX_ASSERT(&ifp->if_flt_lock, LCK_MTX_ASSERT_OWNED);
	TAILQ_INSERT_TAIL(&ifp->if_flt_head, filter, filt_next);

	if_flt_monitor_leave(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	*filter_ref = filter;

	  
                                                        
                                               
    
	if ((filter->filt_flags & DLIL_IFF_TSO) == 0) {
		ifnet_filter_update_tso(TRUE);
	}
	OSIncrementAtomic64(&net_api_stats.nas_iflt_attach_count);
	INC_ATOMIC_INT64_LIM(net_api_stats.nas_iflt_attach_total);
	if ((filter->filt_flags & DLIL_IFF_INTERNAL)) {
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_iflt_attach_os_total);
	}
	if (dlil_verbose) {
		DLIL_PRINTF("%s: %s filter attached\n", if_name(ifp),
		    if_filter->iff_name);
	}
done:
	ifnet_head_done();
	if (retval != 0 && ifp != NULL) {
		DLIL_PRINTF("%s: failed to attach %s (err=%d)\n",
		    if_name(ifp), if_filter->iff_name, retval);
	}
	if (retval != 0 && filter != NULL) {
		zfree(dlif_filt_zone, filter);
	}

	return retval;
}

static int
dlil_detach_filter_internal(interface_filter_t  filter, int detached)
{
	int retval = 0;

	if (detached == 0) {
		ifnet_t ifp = NULL;

		ifnet_head_lock_shared();
		TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
			interface_filter_t entry = NULL;

			lck_mtx_lock(&ifp->if_flt_lock);
			TAILQ_FOREACH(entry, &ifp->if_flt_head, filt_next) {
				if (entry != filter || entry->filt_skip) {
					continue;
				}
				  
                                               
                                                   
                                              
                                                
                                       
       
				entry->filt_skip = 1;                          
				lck_mtx_unlock(&ifp->if_flt_lock);
				ifnet_head_done();

				lck_mtx_lock(&ifp->if_flt_lock);
				if_flt_monitor_enter(ifp);
				LCK_MTX_ASSERT(&ifp->if_flt_lock,
				    LCK_MTX_ASSERT_OWNED);

				                                     
				TAILQ_REMOVE(&ifp->if_flt_head, filter,
				    filt_next);

				if_flt_monitor_leave(ifp);
				lck_mtx_unlock(&ifp->if_flt_lock);
				if (dlil_verbose) {
					DLIL_PRINTF("%s: %s filter detached\n",
					    if_name(ifp), filter->filt_name);
				}
				goto destroy;
			}
			lck_mtx_unlock(&ifp->if_flt_lock);
		}
		ifnet_head_done();

		                                                
		retval = EINVAL;
		goto done;
	}

	if (dlil_verbose) {
		DLIL_PRINTF("%s filter detached\n", filter->filt_name);
	}

destroy:

	                                                
	if (filter->filt_detached) {
		filter->filt_detached(filter->filt_cookie, filter->filt_ifp);
	}

	  
                                                            
                                             
    
	if ((filter->filt_flags & DLIL_IFF_TSO) == 0) {
		ifnet_filter_update_tso(FALSE);
	}

	VERIFY(OSDecrementAtomic64(&net_api_stats.nas_iflt_attach_count) > 0);

	                     
	zfree(dlif_filt_zone, filter);
	filter = NULL;
done:
	if (retval != 0 && filter != NULL) {
		DLIL_PRINTF("failed to detach %s filter (err=%d)\n",
		    filter->filt_name, retval);
	}

	return retval;
}

__private_extern__ void
dlil_detach_filter(interface_filter_t filter)
{
	if (filter == NULL) {
		return;
	}
	dlil_detach_filter_internal(filter, 0);
}

__private_extern__ boolean_t
dlil_has_ip_filter(void)
{
	boolean_t has_filter = (net_api_stats.nas_ipf_add_count > 0);
	DTRACE_IP1(dlil_has_ip_filter, boolean_t, has_filter);
	return has_filter;
}

__private_extern__ boolean_t
dlil_has_if_filter(struct ifnet *ifp)
{
	boolean_t has_filter = !TAILQ_EMPTY(&ifp->if_flt_head);
	DTRACE_IP1(dlil_has_if_filter, boolean_t, has_filter);
	return has_filter;
}

static inline void
dlil_input_wakeup(struct dlil_threading_info *inp)
{
	LCK_MTX_ASSERT(&inp->dlth_lock, LCK_MTX_ASSERT_OWNED);

	inp->dlth_flags |= DLIL_INPUT_WAITING;
	if (!(inp->dlth_flags & DLIL_INPUT_RUNNING)) {
		inp->dlth_wtot++;
		wakeup_one((caddr_t)&inp->dlth_flags);
	}
}

__attribute__((noreturn))
static void
dlil_main_input_thread_func(void *v, wait_result_t w)
{
#pragma unused(w)
	struct dlil_threading_info *inp = v;

	VERIFY(inp == dlil_main_input_thread);
	VERIFY(inp->dlth_ifp == NULL);
	VERIFY(current_thread() == inp->dlth_thread);

	lck_mtx_lock(&inp->dlth_lock);
	VERIFY(!(inp->dlth_flags & (DLIL_INPUT_EMBRYONIC | DLIL_INPUT_RUNNING)));
	(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
	inp->dlth_flags |= DLIL_INPUT_EMBRYONIC;
	                                                
	dlil_input_wakeup(inp);
	lck_mtx_unlock(&inp->dlth_lock);
	(void) thread_block_parameter(dlil_main_input_thread_cont, inp);
	                
	__builtin_unreachable();
}

  
                     
  
                                           
                                                                    
                                                                     
                          
                              
                         
   
__attribute__((noreturn))
static void
dlil_main_input_thread_cont(void *v, wait_result_t wres)
{
	struct dlil_main_threading_info *inpm = v;
	struct dlil_threading_info *inp = v;

	                                          
	VERIFY(wres != THREAD_INTERRUPTED);
	lck_mtx_lock_spin(&inp->dlth_lock);
	VERIFY(!(inp->dlth_flags & (DLIL_INPUT_TERMINATE |
	    DLIL_INPUT_RUNNING)));
	inp->dlth_flags |= DLIL_INPUT_RUNNING;

	while (1) {
		struct mbuf *m = NULL, *m_loop = NULL;
		u_int32_t m_cnt, m_cnt_loop;
		classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);
		boolean_t proto_req;
		boolean_t embryonic;

		inp->dlth_flags &= ~DLIL_INPUT_WAITING;

		if (__improbable(embryonic =
		    (inp->dlth_flags & DLIL_INPUT_EMBRYONIC))) {
			inp->dlth_flags &= ~DLIL_INPUT_EMBRYONIC;
		}

		proto_req = (inp->dlth_flags &
		    (DLIL_PROTO_WAITING | DLIL_PROTO_REGISTER));

		                                                         
		m_cnt = qlen(&inp->dlth_pkts);
		_getq_all(&inp->dlth_pkts, &pkt, NULL, NULL, NULL);
		m = pkt.cp_mbuf;

		                              
		m_cnt_loop = qlen(&inpm->lo_rcvq_pkts);
		_getq_all(&inpm->lo_rcvq_pkts, &pkt, NULL, NULL, NULL);
		m_loop = pkt.cp_mbuf;

		inp->dlth_wtot = 0;

		lck_mtx_unlock(&inp->dlth_lock);

		if (__improbable(embryonic)) {
			dlil_decr_pending_thread_count();
		}

		  
                                    
                                                         
                                                       
     
		if (__probable(m_loop != NULL)) {
			dlil_input_packet_list_extended(lo_ifp, m_loop,
			    m_cnt_loop, IFNET_MODEL_INPUT_POLL_OFF);
		}

		if (__probable(m != NULL)) {
			dlil_input_packet_list_extended(NULL, m,
			    m_cnt, IFNET_MODEL_INPUT_POLL_OFF);
		}

		if (__improbable(proto_req)) {
			proto_input_run();
		}

		lck_mtx_lock_spin(&inp->dlth_lock);
		VERIFY(inp->dlth_flags & DLIL_INPUT_RUNNING);
		                                            
		VERIFY(!(inp->dlth_flags & DLIL_INPUT_TERMINATE));
		if (!(inp->dlth_flags & ~DLIL_INPUT_RUNNING)) {
			break;
		}
	}

	inp->dlth_flags &= ~DLIL_INPUT_RUNNING;
	(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
	lck_mtx_unlock(&inp->dlth_lock);
	(void) thread_block_parameter(dlil_main_input_thread_cont, inp);

	VERIFY(0);                                    
	                
	__builtin_unreachable();
}

  
                                                       
   
__attribute__((noreturn))
static void
dlil_input_thread_func(void *v, wait_result_t w)
{
#pragma unused(w)
	char thread_name[MAXTHREADNAMESIZE];
	struct dlil_threading_info *inp = v;
	struct ifnet *ifp = inp->dlth_ifp;

	VERIFY(inp != dlil_main_input_thread);
	VERIFY(ifp != NULL);
	VERIFY(!(ifp->if_eflags & IFEF_RXPOLL) || !net_rxpoll ||
	    !(ifp->if_xflags & IFXF_LEGACY));
	VERIFY(ifp->if_poll_mode == IFNET_MODEL_INPUT_POLL_OFF ||
	    !(ifp->if_xflags & IFXF_LEGACY));
	VERIFY(current_thread() == inp->dlth_thread);

	                                                           
	bzero(thread_name, sizeof(thread_name));
	(void) snprintf(thread_name, sizeof(thread_name),
	    "dlil_input_%s", ifp->if_xname);
	thread_set_thread_name(inp->dlth_thread, thread_name);

	lck_mtx_lock(&inp->dlth_lock);
	VERIFY(!(inp->dlth_flags & (DLIL_INPUT_EMBRYONIC | DLIL_INPUT_RUNNING)));
	(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
	inp->dlth_flags |= DLIL_INPUT_EMBRYONIC;
	                                                
	dlil_input_wakeup(inp);
	lck_mtx_unlock(&inp->dlth_lock);
	(void) thread_block_parameter(dlil_input_thread_cont, inp);
	                
	__builtin_unreachable();
}

__attribute__((noreturn))
static void
dlil_input_thread_cont(void *v, wait_result_t wres)
{
	struct dlil_threading_info *inp = v;
	struct ifnet *ifp = inp->dlth_ifp;

	lck_mtx_lock_spin(&inp->dlth_lock);
	if (__improbable(wres == THREAD_INTERRUPTED ||
	    (inp->dlth_flags & DLIL_INPUT_TERMINATE))) {
		goto terminate;
	}

	VERIFY(!(inp->dlth_flags & DLIL_INPUT_RUNNING));
	inp->dlth_flags |= DLIL_INPUT_RUNNING;

	while (1) {
		struct mbuf *m = NULL;
		classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);
		boolean_t notify = FALSE;
		boolean_t embryonic;
		u_int32_t m_cnt;

		inp->dlth_flags &= ~DLIL_INPUT_WAITING;

		if (__improbable(embryonic =
		    (inp->dlth_flags & DLIL_INPUT_EMBRYONIC))) {
			inp->dlth_flags &= ~DLIL_INPUT_EMBRYONIC;
		}

		  
                                                        
                                                            
                                                            
                                                               
                                                    
     
		VERIFY(!(inp->dlth_flags &
		    (DLIL_PROTO_WAITING | DLIL_PROTO_REGISTER)));

		                                
		m_cnt = qlen(&inp->dlth_pkts);
		_getq_all(&inp->dlth_pkts, &pkt, NULL, NULL, NULL);
		m = pkt.cp_mbuf;

		inp->dlth_wtot = 0;

		notify = dlil_input_stats_sync(ifp, inp);

		lck_mtx_unlock(&inp->dlth_lock);

		if (__improbable(embryonic)) {
			ifnet_decr_pending_thread_count(ifp);
		}

		if (__improbable(notify)) {
			ifnet_notify_data_threshold(ifp);
		}

		  
                                    
                                                         
                                                       
     
		if (__probable(m != NULL)) {
			dlil_input_packet_list_extended(NULL, m,
			    m_cnt, ifp->if_poll_mode);
		}

		lck_mtx_lock_spin(&inp->dlth_lock);
		VERIFY(inp->dlth_flags & DLIL_INPUT_RUNNING);
		if (!(inp->dlth_flags & ~(DLIL_INPUT_RUNNING |
		    DLIL_INPUT_TERMINATE))) {
			break;
		}
	}

	inp->dlth_flags &= ~DLIL_INPUT_RUNNING;

	if (__improbable(inp->dlth_flags & DLIL_INPUT_TERMINATE)) {
terminate:
		lck_mtx_unlock(&inp->dlth_lock);
		dlil_terminate_input_thread(inp);
		                
	} else {
		(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
		lck_mtx_unlock(&inp->dlth_lock);
		(void) thread_block_parameter(dlil_input_thread_cont, inp);
		                
	}

	VERIFY(0);                                    
	                
	__builtin_unreachable();
}

  
                                                                      
   
__attribute__((noreturn))
static void
dlil_rxpoll_input_thread_func(void *v, wait_result_t w)
{
#pragma unused(w)
	char thread_name[MAXTHREADNAMESIZE];
	struct dlil_threading_info *inp = v;
	struct ifnet *ifp = inp->dlth_ifp;

	VERIFY(inp != dlil_main_input_thread);
	VERIFY(ifp != NULL && (ifp->if_eflags & IFEF_RXPOLL) &&
	    (ifp->if_xflags & IFXF_LEGACY));
	VERIFY(current_thread() == inp->dlth_thread);

	                                                           
	bzero(thread_name, sizeof(thread_name));
	(void) snprintf(thread_name, sizeof(thread_name),
	    "dlil_input_poll_%s", ifp->if_xname);
	thread_set_thread_name(inp->dlth_thread, thread_name);

	lck_mtx_lock(&inp->dlth_lock);
	VERIFY(!(inp->dlth_flags & (DLIL_INPUT_EMBRYONIC | DLIL_INPUT_RUNNING)));
	(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
	inp->dlth_flags |= DLIL_INPUT_EMBRYONIC;
	                                                
	dlil_input_wakeup(inp);
	lck_mtx_unlock(&inp->dlth_lock);
	(void) thread_block_parameter(dlil_rxpoll_input_thread_cont, inp);
	                
	__builtin_unreachable();
}

__attribute__((noreturn))
static void
dlil_rxpoll_input_thread_cont(void *v, wait_result_t wres)
{
	struct dlil_threading_info *inp = v;
	struct ifnet *ifp = inp->dlth_ifp;
	struct timespec ts;

	lck_mtx_lock_spin(&inp->dlth_lock);
	if (__improbable(wres == THREAD_INTERRUPTED ||
	    (inp->dlth_flags & DLIL_INPUT_TERMINATE))) {
		goto terminate;
	}

	VERIFY(!(inp->dlth_flags & DLIL_INPUT_RUNNING));
	inp->dlth_flags |= DLIL_INPUT_RUNNING;

	while (1) {
		struct mbuf *m = NULL;
		uint32_t m_cnt, poll_req = 0;
		uint64_t m_size = 0;
		ifnet_model_t mode;
		struct timespec now, delta;
		classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);
		boolean_t notify;
		boolean_t embryonic;
		uint64_t ival;

		inp->dlth_flags &= ~DLIL_INPUT_WAITING;

		if (__improbable(embryonic =
		    (inp->dlth_flags & DLIL_INPUT_EMBRYONIC))) {
			inp->dlth_flags &= ~DLIL_INPUT_EMBRYONIC;
			goto skip;
		}

		if ((ival = ifp->if_rxpoll_ival) < IF_RXPOLL_INTERVALTIME_MIN) {
			ival = IF_RXPOLL_INTERVALTIME_MIN;
		}

		                              
		if (ifp->if_poll_update != 0) {
			ifp->if_poll_update = 0;
			(void) dlil_rxpoll_set_params(ifp, NULL, TRUE);
		}

		                            
		mode = ifp->if_poll_mode;

		  
                                                        
                                                            
                                                            
                                                               
                                                    
     
		VERIFY(!(inp->dlth_flags &
		    (DLIL_PROTO_WAITING | DLIL_PROTO_REGISTER)));

		                                
		m_cnt = qlen(&inp->dlth_pkts);

		                                
		m_size = qsize(&inp->dlth_pkts);

		                                
		_getq_all(&inp->dlth_pkts, &pkt, NULL, NULL, NULL);
		m = pkt.cp_mbuf;
		VERIFY(m != NULL || m_cnt == 0);

		nanouptime(&now);
		if (!net_timerisset(&ifp->if_poll_sample_lasttime)) {
			*(&ifp->if_poll_sample_lasttime) = *(&now);
		}

		net_timersub(&now, &ifp->if_poll_sample_lasttime, &delta);
		if (if_rxpoll && net_timerisset(&ifp->if_poll_sample_holdtime)) {
			u_int32_t ptot, btot;

			                                                
			PKTCNTR_ADD(&ifp->if_poll_sstats, m_cnt, m_size);

			if (net_timercmp(&delta, &ifp->if_poll_sample_holdtime, <)) {
				goto skip;
			}

			*(&ifp->if_poll_sample_lasttime) = *(&now);

			                                        
			btot = (u_int32_t)ifp->if_poll_sstats.bytes;
			if (ifp->if_rxpoll_bmin == 0 || ifp->if_rxpoll_bmin > btot) {
				ifp->if_rxpoll_bmin = btot;
			}
			if (btot > ifp->if_rxpoll_bmax) {
				ifp->if_rxpoll_bmax = btot;
			}

			                                     
			DLIL_EWMA(ifp->if_rxpoll_bavg, btot, if_rxpoll_decay);

			                                          
			ptot = (u_int32_t)ifp->if_poll_sstats.packets;
			if (ifp->if_rxpoll_pmin == 0 || ifp->if_rxpoll_pmin > ptot) {
				ifp->if_rxpoll_pmin = ptot;
			}
			if (ptot > ifp->if_rxpoll_pmax) {
				ifp->if_rxpoll_pmax = ptot;
			}

			                                       
			DLIL_EWMA(ifp->if_rxpoll_pavg, ptot, if_rxpoll_decay);

			                               
			PKTCNTR_CLEAR(&ifp->if_poll_sstats);

			                                       
			DLIL_EWMA(ifp->if_rxpoll_wavg, inp->dlth_wtot,
			    if_rxpoll_decay);
			inp->dlth_wtot = 0;

			if (dlil_verbose) {
				if (!net_timerisset(&ifp->if_poll_dbg_lasttime)) {
					*(&ifp->if_poll_dbg_lasttime) = *(&now);
				}
				net_timersub(&now, &ifp->if_poll_dbg_lasttime, &delta);
				if (net_timercmp(&delta, &dlil_dbgrate, >=)) {
					*(&ifp->if_poll_dbg_lasttime) = *(&now);
					DLIL_PRINTF("%s: [%s] pkts avg %d max %d "
					    "limits [%d/%d], wreq avg %d "
					    "limits [%d/%d], bytes avg %d "
					    "limits [%d/%d]\n", if_name(ifp),
					    (ifp->if_poll_mode ==
					    IFNET_MODEL_INPUT_POLL_ON) ?
					    "ON" : "OFF", ifp->if_rxpoll_pavg,
					    ifp->if_rxpoll_pmax,
					    ifp->if_rxpoll_plowat,
					    ifp->if_rxpoll_phiwat,
					    ifp->if_rxpoll_wavg,
					    ifp->if_rxpoll_wlowat,
					    ifp->if_rxpoll_whiwat,
					    ifp->if_rxpoll_bavg,
					    ifp->if_rxpoll_blowat,
					    ifp->if_rxpoll_bhiwat);
				}
			}

			                                           
			if (!net_timerisset(&ifp->if_poll_mode_lasttime)) {
				*(&ifp->if_poll_mode_lasttime) = *(&now);
			}

			net_timersub(&now, &ifp->if_poll_mode_lasttime, &delta);
			if (net_timercmp(&delta, &ifp->if_poll_mode_holdtime, <)) {
				goto skip;
			}

			if (ifp->if_rxpoll_pavg <= ifp->if_rxpoll_plowat &&
			    ifp->if_rxpoll_bavg <= ifp->if_rxpoll_blowat &&
			    ifp->if_poll_mode != IFNET_MODEL_INPUT_POLL_OFF) {
				mode = IFNET_MODEL_INPUT_POLL_OFF;
			} else if (ifp->if_rxpoll_pavg >= ifp->if_rxpoll_phiwat &&
			    (ifp->if_rxpoll_bavg >= ifp->if_rxpoll_bhiwat ||
			    ifp->if_rxpoll_wavg >= ifp->if_rxpoll_whiwat) &&
			    ifp->if_poll_mode != IFNET_MODEL_INPUT_POLL_ON) {
				mode = IFNET_MODEL_INPUT_POLL_ON;
			}

			if (mode != ifp->if_poll_mode) {
				ifp->if_poll_mode = mode;
				*(&ifp->if_poll_mode_lasttime) = *(&now);
				poll_req++;
			}
		}
skip:
		notify = dlil_input_stats_sync(ifp, inp);

		lck_mtx_unlock(&inp->dlth_lock);

		if (__improbable(embryonic)) {
			ifnet_decr_pending_thread_count(ifp);
		}

		if (__improbable(notify)) {
			ifnet_notify_data_threshold(ifp);
		}

		  
                                                              
                                                             
                                                          
                                            
     
		if (poll_req != 0 && ifnet_is_attached(ifp, 1)) {
			struct ifnet_model_params p = {
				.model = mode, .reserved = { 0 }
			};
			errno_t err;

			if (dlil_verbose) {
				DLIL_PRINTF("%s: polling is now %s, "
				    "pkts avg %d max %d limits [%d/%d], "
				    "wreq avg %d limits [%d/%d], "
				    "bytes avg %d limits [%d/%d]\n",
				    if_name(ifp),
				    (mode == IFNET_MODEL_INPUT_POLL_ON) ?
				    "ON" : "OFF", ifp->if_rxpoll_pavg,
				    ifp->if_rxpoll_pmax, ifp->if_rxpoll_plowat,
				    ifp->if_rxpoll_phiwat, ifp->if_rxpoll_wavg,
				    ifp->if_rxpoll_wlowat, ifp->if_rxpoll_whiwat,
				    ifp->if_rxpoll_bavg, ifp->if_rxpoll_blowat,
				    ifp->if_rxpoll_bhiwat);
			}

			if ((err = ((*ifp->if_input_ctl)(ifp,
			    IFNET_CTL_SET_INPUT_MODEL, sizeof(p), &p))) != 0) {
				DLIL_PRINTF("%s: error setting polling mode "
				    "to %s (%d)\n", if_name(ifp),
				    (mode == IFNET_MODEL_INPUT_POLL_ON) ?
				    "ON" : "OFF", err);
			}

			switch (mode) {
			case IFNET_MODEL_INPUT_POLL_OFF:
				ifnet_set_poll_cycle(ifp, NULL);
				ifp->if_rxpoll_offreq++;
				if (err != 0) {
					ifp->if_rxpoll_offerr++;
				}
				break;

			case IFNET_MODEL_INPUT_POLL_ON:
				net_nsectimer(&ival, &ts);
				ifnet_set_poll_cycle(ifp, &ts);
				ifnet_poll(ifp);
				ifp->if_rxpoll_onreq++;
				if (err != 0) {
					ifp->if_rxpoll_onerr++;
				}
				break;

			default:
				VERIFY(0);
				                
			}

			                           
			ifnet_decr_iorefcnt(ifp);
		}

		  
                                    
                                                         
                                                       
     
		if (__probable(m != NULL)) {
			dlil_input_packet_list_extended(NULL, m, m_cnt, mode);
		}

		lck_mtx_lock_spin(&inp->dlth_lock);
		VERIFY(inp->dlth_flags & DLIL_INPUT_RUNNING);
		if (!(inp->dlth_flags & ~(DLIL_INPUT_RUNNING |
		    DLIL_INPUT_TERMINATE))) {
			break;
		}
	}

	inp->dlth_flags &= ~DLIL_INPUT_RUNNING;

	if (__improbable(inp->dlth_flags & DLIL_INPUT_TERMINATE)) {
terminate:
		lck_mtx_unlock(&inp->dlth_lock);
		dlil_terminate_input_thread(inp);
		                
	} else {
		(void) assert_wait(&inp->dlth_flags, THREAD_UNINT);
		lck_mtx_unlock(&inp->dlth_lock);
		(void) thread_block_parameter(dlil_rxpoll_input_thread_cont,
		    inp);
		                
	}

	VERIFY(0);                                    
	                
	__builtin_unreachable();
}

errno_t
dlil_rxpoll_validate_params(struct ifnet_poll_params *p)
{
	if (p != NULL) {
		if ((p->packets_lowat == 0 && p->packets_hiwat != 0) ||
		    (p->packets_lowat != 0 && p->packets_hiwat == 0)) {
			return EINVAL;
		}
		if (p->packets_lowat != 0 &&                                
		    p->packets_lowat >= p->packets_hiwat) {
			return EINVAL;
		}
		if ((p->bytes_lowat == 0 && p->bytes_hiwat != 0) ||
		    (p->bytes_lowat != 0 && p->bytes_hiwat == 0)) {
			return EINVAL;
		}
		if (p->bytes_lowat != 0 &&                                  
		    p->bytes_lowat >= p->bytes_hiwat) {
			return EINVAL;
		}
		if (p->interval_time != 0 &&
		    p->interval_time < IF_RXPOLL_INTERVALTIME_MIN) {
			p->interval_time = IF_RXPOLL_INTERVALTIME_MIN;
		}
	}
	return 0;
}

void
dlil_rxpoll_update_params(struct ifnet *ifp, struct ifnet_poll_params *p)
{
	u_int64_t sample_holdtime, inbw;

	if ((inbw = ifnet_input_linkrate(ifp)) == 0 && p == NULL) {
		sample_holdtime = 0;                             
		ifp->if_rxpoll_wlowat = ifp->if_rxpoll_plowat =
		    ifp->if_rxpoll_blowat = 0;
		ifp->if_rxpoll_whiwat = ifp->if_rxpoll_phiwat =
		    ifp->if_rxpoll_bhiwat = (u_int32_t)-1;
		ifp->if_rxpoll_plim = 0;
		ifp->if_rxpoll_ival = IF_RXPOLL_INTERVALTIME_MIN;
	} else {
		u_int32_t plowat, phiwat, blowat, bhiwat, plim;
		u_int64_t ival;
		unsigned int n, i;

		for (n = 0, i = 0; rxpoll_tbl[i].speed != 0; i++) {
			if (inbw < rxpoll_tbl[i].speed) {
				break;
			}
			n = i;
		}
		                                                
		plowat = ((p == NULL || p->packets_lowat == 0) ?
		    rxpoll_tbl[n].plowat : p->packets_lowat);
		phiwat = ((p == NULL || p->packets_hiwat == 0) ?
		    rxpoll_tbl[n].phiwat : p->packets_hiwat);
		blowat = ((p == NULL || p->bytes_lowat == 0) ?
		    rxpoll_tbl[n].blowat : p->bytes_lowat);
		bhiwat = ((p == NULL || p->bytes_hiwat == 0) ?
		    rxpoll_tbl[n].bhiwat : p->bytes_hiwat);
		plim = ((p == NULL || p->packets_limit == 0) ?
		    if_rxpoll_max : p->packets_limit);
		ival = ((p == NULL || p->interval_time == 0) ?
		    if_rxpoll_interval_time : p->interval_time);

		VERIFY(plowat != 0 && phiwat != 0);
		VERIFY(blowat != 0 && bhiwat != 0);
		VERIFY(ival >= IF_RXPOLL_INTERVALTIME_MIN);

		sample_holdtime = if_rxpoll_sample_holdtime;
		ifp->if_rxpoll_wlowat = if_sysctl_rxpoll_wlowat;
		ifp->if_rxpoll_whiwat = if_sysctl_rxpoll_whiwat;
		ifp->if_rxpoll_plowat = plowat;
		ifp->if_rxpoll_phiwat = phiwat;
		ifp->if_rxpoll_blowat = blowat;
		ifp->if_rxpoll_bhiwat = bhiwat;
		ifp->if_rxpoll_plim = plim;
		ifp->if_rxpoll_ival = ival;
	}

	net_nsectimer(&if_rxpoll_mode_holdtime, &ifp->if_poll_mode_holdtime);
	net_nsectimer(&sample_holdtime, &ifp->if_poll_sample_holdtime);

	if (dlil_verbose) {
		DLIL_PRINTF("%s: speed %llu bps, sample per %llu nsec, "
		    "poll interval %llu nsec, pkts per poll %u, "
		    "pkt limits [%u/%u], wreq limits [%u/%u], "
		    "bytes limits [%u/%u]\n", if_name(ifp),
		    inbw, sample_holdtime, ifp->if_rxpoll_ival,
		    ifp->if_rxpoll_plim, ifp->if_rxpoll_plowat,
		    ifp->if_rxpoll_phiwat, ifp->if_rxpoll_wlowat,
		    ifp->if_rxpoll_whiwat, ifp->if_rxpoll_blowat,
		    ifp->if_rxpoll_bhiwat);
	}
}

  
                                                                     
                                                                      
   
errno_t
dlil_rxpoll_set_params(struct ifnet *ifp, struct ifnet_poll_params *p,
    boolean_t locked)
{
	errno_t err;
	struct dlil_threading_info *inp;

	VERIFY(ifp != NULL);
	if (!(ifp->if_eflags & IFEF_RXPOLL) || (inp = ifp->if_inp) == NULL) {
		return ENXIO;
	}
	err = dlil_rxpoll_validate_params(p);
	if (err != 0) {
		return err;
	}

	if (!locked) {
		lck_mtx_lock(&inp->dlth_lock);
	}
	LCK_MTX_ASSERT(&inp->dlth_lock, LCK_MTX_ASSERT_OWNED);
	  
                                                                
                                                                  
                                                              
                                                                
                                                                   
                      
    
	if (p != NULL && !locked && ifp->if_poll_update != 0) {
		ifp->if_poll_update = 0;
	}
	dlil_rxpoll_update_params(ifp, p);
	if (!locked) {
		lck_mtx_unlock(&inp->dlth_lock);
	}
	return 0;
}

  
                                                                     
   
errno_t
dlil_rxpoll_get_params(struct ifnet *ifp, struct ifnet_poll_params *p)
{
	struct dlil_threading_info *inp;

	VERIFY(ifp != NULL && p != NULL);
	if (!(ifp->if_eflags & IFEF_RXPOLL) || (inp = ifp->if_inp) == NULL) {
		return ENXIO;
	}

	bzero(p, sizeof(*p));

	lck_mtx_lock(&inp->dlth_lock);
	p->packets_limit = ifp->if_rxpoll_plim;
	p->packets_lowat = ifp->if_rxpoll_plowat;
	p->packets_hiwat = ifp->if_rxpoll_phiwat;
	p->bytes_lowat = ifp->if_rxpoll_blowat;
	p->bytes_hiwat = ifp->if_rxpoll_bhiwat;
	p->interval_time = ifp->if_rxpoll_ival;
	lck_mtx_unlock(&inp->dlth_lock);

	return 0;
}

errno_t
ifnet_input(struct ifnet *ifp, struct mbuf *m_head,
    const struct ifnet_stat_increment_param *s)
{
	return ifnet_input_common(ifp, m_head, NULL, s, FALSE, FALSE);
}

errno_t
ifnet_input_extended(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, const struct ifnet_stat_increment_param *s)
{
	return ifnet_input_common(ifp, m_head, m_tail, s, TRUE, FALSE);
}

errno_t
ifnet_input_poll(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, const struct ifnet_stat_increment_param *s)
{
	return ifnet_input_common(ifp, m_head, m_tail, s,
	           (m_head != NULL), TRUE);
}

static errno_t
ifnet_input_common(struct ifnet *ifp, struct mbuf *m_head, struct mbuf *m_tail,
    const struct ifnet_stat_increment_param *s, boolean_t ext, boolean_t poll)
{
	dlil_input_func input_func;
	struct ifnet_stat_increment_param _s;
	u_int32_t m_cnt = 0, m_size = 0;
	struct mbuf *last;
	errno_t err = 0;

	if ((m_head == NULL && !poll) || (s == NULL && ext)) {
		if (m_head != NULL) {
			mbuf_freem_list(m_head);
		}
		return EINVAL;
	}

	VERIFY(m_head != NULL || (s == NULL && m_tail == NULL && !ext && poll));
	VERIFY(m_tail == NULL || ext);
	VERIFY(s != NULL || !ext);

	  
                                                               
                                                              
                                                            
    
	if (ifp == NULL || (ifp != lo_ifp && !ifnet_datamov_begin(ifp))) {
		if (m_head != NULL) {
			mbuf_freem_list(m_head);
		}
		return EINVAL;
	}

	input_func = ifp->if_input_dlil;
	VERIFY(input_func != NULL);

	if (m_tail == NULL) {
		last = m_head;
		while (m_head != NULL) {
#if IFNET_INPUT_SANITY_CHK
			if (__improbable(dlil_input_sanity_check != 0)) {
				DLIL_INPUT_CHECK(last, ifp);
			}
#endif                             
			m_cnt++;
			m_size += m_length(last);
			if (mbuf_nextpkt(last) == NULL) {
				break;
			}
			last = mbuf_nextpkt(last);
		}
		m_tail = last;
	} else {
#if IFNET_INPUT_SANITY_CHK
		if (__improbable(dlil_input_sanity_check != 0)) {
			last = m_head;
			while (1) {
				DLIL_INPUT_CHECK(last, ifp);
				m_cnt++;
				m_size += m_length(last);
				if (mbuf_nextpkt(last) == NULL) {
					break;
				}
				last = mbuf_nextpkt(last);
			}
		} else {
			m_cnt = s->packets_in;
			m_size = s->bytes_in;
			last = m_tail;
		}
#else
		m_cnt = s->packets_in;
		m_size = s->bytes_in;
		last = m_tail;
#endif                             
	}

	if (last != m_tail) {
		panic_plain("%s: invalid input packet chain for %s, "
		    "tail mbuf %p instead of %p\n", __func__, if_name(ifp),
		    m_tail, last);
	}

	  
                                                                    
                                                                   
                                                                
                                                             
                                    
    
	if (ext && s->packets_in != m_cnt) {
		panic_plain("%s: input packet count mismatch for %s, "
		    "%d instead of %d\n", __func__, if_name(ifp),
		    s->packets_in, m_cnt);
	}

	if (s == NULL) {
		bzero(&_s, sizeof(_s));
		s = &_s;
	} else {
		_s = *s;
	}
	_s.packets_in = m_cnt;
	_s.bytes_in = m_size;

	err = (*input_func)(ifp, m_head, m_tail, s, poll, current_thread());

	if (ifp != lo_ifp) {
		                           
		ifnet_datamov_end(ifp);
	}

	return err;
}


errno_t
dlil_output_handler(struct ifnet *ifp, struct mbuf *m)
{
	return ifp->if_output(ifp, m);
}

errno_t
dlil_input_handler(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, const struct ifnet_stat_increment_param *s,
    boolean_t poll, struct thread *tp)
{
	struct dlil_threading_info *inp = ifp->if_inp;

	if (__improbable(inp == NULL)) {
		inp = dlil_main_input_thread;
	}

	return inp->dlth_strategy(inp, ifp, m_head, m_tail, s, poll, tp);
}

static errno_t
dlil_input_async(struct dlil_threading_info *inp,
    struct ifnet *ifp, struct mbuf *m_head, struct mbuf *m_tail,
    const struct ifnet_stat_increment_param *s, boolean_t poll,
    struct thread *tp)
{
	u_int32_t m_cnt = s->packets_in;
	u_int32_t m_size = s->bytes_in;
	boolean_t notify = FALSE;

	  
                                                               
                                                              
                           
    
	lck_mtx_lock_spin(&inp->dlth_lock);
	if (inp != dlil_main_input_thread && inp->dlth_affinity && tp != NULL &&
	    ((!poll && inp->dlth_driver_thread == THREAD_NULL) ||
	    (poll && inp->dlth_poller_thread == THREAD_NULL))) {
		u_int32_t tag = inp->dlth_affinity_tag;

		if (poll) {
			VERIFY(inp->dlth_poller_thread == THREAD_NULL);
			inp->dlth_poller_thread = tp;
		} else {
			VERIFY(inp->dlth_driver_thread == THREAD_NULL);
			inp->dlth_driver_thread = tp;
		}
		lck_mtx_unlock(&inp->dlth_lock);

		                                                            
		(void) dlil_affinity_set(tp, tag);

		  
                                                           
                                                          
              
     
		thread_reference(tp);
		lck_mtx_lock_spin(&inp->dlth_lock);
	}

	VERIFY(m_head != NULL || (m_tail == NULL && m_cnt == 0));

	  
                                                              
                                                                
                                                                  
                                                                    
                                                   
    
	if (m_head != NULL) {
		classq_pkt_t head, tail;
		CLASSQ_PKT_INIT_MBUF(&head, m_head);
		CLASSQ_PKT_INIT_MBUF(&tail, m_tail);
		if (inp == dlil_main_input_thread && ifp == lo_ifp) {
			struct dlil_main_threading_info *inpm =
			    (struct dlil_main_threading_info *)inp;
			_addq_multi(&inpm->lo_rcvq_pkts, &head, &tail,
			    m_cnt, m_size);
		} else {
			_addq_multi(&inp->dlth_pkts, &head, &tail,
			    m_cnt, m_size);
		}
	}

#if IFNET_INPUT_SANITY_CHK
	if (__improbable(dlil_input_sanity_check != 0)) {
		u_int32_t count = 0, size = 0;
		struct mbuf *m0;

		for (m0 = m_head; m0; m0 = mbuf_nextpkt(m0)) {
			size += m_length(m0);
			count++;
		}

		if (count != m_cnt) {
			panic_plain("%s: invalid total packet count %u "
			    "(expected %u)\n", if_name(ifp), count, m_cnt);
			                
			__builtin_unreachable();
		} else if (size != m_size) {
			panic_plain("%s: invalid total packet size %u "
			    "(expected %u)\n", if_name(ifp), size, m_size);
			                
			__builtin_unreachable();
		}

		inp->dlth_pkts_cnt += m_cnt;
	}
#endif                             

	dlil_input_stats_add(s, inp, ifp, poll);
	  
                                                         
                                                       
                                                      
                                        
    
	if (inp == dlil_main_input_thread) {
		notify = dlil_input_stats_sync(ifp, inp);
	}

	dlil_input_wakeup(inp);
	lck_mtx_unlock(&inp->dlth_lock);

	if (notify) {
		ifnet_notify_data_threshold(ifp);
	}

	return 0;
}

static errno_t
dlil_input_sync(struct dlil_threading_info *inp,
    struct ifnet *ifp, struct mbuf *m_head, struct mbuf *m_tail,
    const struct ifnet_stat_increment_param *s, boolean_t poll,
    struct thread *tp)
{
#pragma unused(tp)
	u_int32_t m_cnt = s->packets_in;
	u_int32_t m_size = s->bytes_in;
	boolean_t notify = FALSE;
	classq_pkt_t head, tail;

	ASSERT(inp != dlil_main_input_thread);

	                                         
	if (__improbable(m_head == NULL)) {
		return 0;
	}

	CLASSQ_PKT_INIT_MBUF(&head, m_head);
	CLASSQ_PKT_INIT_MBUF(&tail, m_tail);

	lck_mtx_lock_spin(&inp->dlth_lock);
	_addq_multi(&inp->dlth_pkts, &head, &tail, m_cnt, m_size);

#if IFNET_INPUT_SANITY_CHK
	if (__improbable(dlil_input_sanity_check != 0)) {
		u_int32_t count = 0, size = 0;
		struct mbuf *m0;

		for (m0 = m_head; m0; m0 = mbuf_nextpkt(m0)) {
			size += m_length(m0);
			count++;
		}

		if (count != m_cnt) {
			panic_plain("%s: invalid total packet count %u "
			    "(expected %u)\n", if_name(ifp), count, m_cnt);
			                
			__builtin_unreachable();
		} else if (size != m_size) {
			panic_plain("%s: invalid total packet size %u "
			    "(expected %u)\n", if_name(ifp), size, m_size);
			                
			__builtin_unreachable();
		}

		inp->dlth_pkts_cnt += m_cnt;
	}
#endif                             

	dlil_input_stats_add(s, inp, ifp, poll);

	m_cnt = qlen(&inp->dlth_pkts);
	_getq_all(&inp->dlth_pkts, &head, NULL, NULL, NULL);

	notify = dlil_input_stats_sync(ifp, inp);

	lck_mtx_unlock(&inp->dlth_lock);

	if (notify) {
		ifnet_notify_data_threshold(ifp);
	}

	  
                                   
                                                        
                                                      
    
	if (head.cp_mbuf != NULL) {
		dlil_input_packet_list_extended(NULL, head.cp_mbuf,
		    m_cnt, ifp->if_poll_mode);
	}

	return 0;
}


static void
ifnet_start_common(struct ifnet *ifp, boolean_t resetfc)
{
	if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return;
	}
	  
                                                            
                                                             
                                                            
                                                           
                                     
    
	lck_mtx_lock_spin(&ifp->if_start_lock);
	if (resetfc) {
		ifp->if_start_flags &= ~IFSF_FLOW_CONTROLLED;
	} else if (ifp->if_start_flags & IFSF_FLOW_CONTROLLED) {
		lck_mtx_unlock(&ifp->if_start_lock);
		return;
	}
	ifp->if_start_req++;
	if (!ifp->if_start_active && ifp->if_start_thread != THREAD_NULL &&
	    (resetfc || !(ifp->if_eflags & IFEF_ENQUEUE_MULTI) ||
	    IFCQ_LEN(&ifp->if_snd) >= ifp->if_start_delay_qlen ||
	    ifp->if_start_delayed == 0)) {
		(void) wakeup_one((caddr_t)&ifp->if_start_thread);
	}
	lck_mtx_unlock(&ifp->if_start_lock);
}

void
ifnet_start(struct ifnet *ifp)
{
	ifnet_start_common(ifp, FALSE);
}

__attribute__((noreturn))
static void
ifnet_start_thread_func(void *v, wait_result_t w)
{
#pragma unused(w)
	struct ifnet *ifp = v;
	char thread_name[MAXTHREADNAMESIZE];

	                                                            
	bzero(thread_name, sizeof(thread_name));
	(void) snprintf(thread_name, sizeof(thread_name),
	    "ifnet_start_%s", ifp->if_xname);
	ASSERT(ifp->if_start_thread == current_thread());
	thread_set_thread_name(current_thread(), thread_name);

	  
                                                               
                                                              
                                                              
                                                             
                                                                 
    
	if (ifp == lo_ifp) {
		struct dlil_threading_info *inp = dlil_main_input_thread;
		struct thread *tp = current_thread();

		lck_mtx_lock(&inp->dlth_lock);
		if (inp->dlth_affinity) {
			u_int32_t tag = inp->dlth_affinity_tag;

			VERIFY(inp->dlth_driver_thread == THREAD_NULL);
			VERIFY(inp->dlth_poller_thread == THREAD_NULL);
			inp->dlth_driver_thread = tp;
			lck_mtx_unlock(&inp->dlth_lock);

			                                                 
			(void) dlil_affinity_set(tp, tag);
		} else {
			lck_mtx_unlock(&inp->dlth_lock);
		}
	}

	lck_mtx_lock(&ifp->if_start_lock);
	VERIFY(!ifp->if_start_embryonic && !ifp->if_start_active);
	(void) assert_wait(&ifp->if_start_thread, THREAD_UNINT);
	ifp->if_start_embryonic = 1;
	                                                
	ifp->if_start_req++;
	(void) wakeup_one((caddr_t)&ifp->if_start_thread);
	lck_mtx_unlock(&ifp->if_start_lock);
	(void) thread_block_parameter(ifnet_start_thread_cont, ifp);
	                
	__builtin_unreachable();
}

__attribute__((noreturn))
static void
ifnet_start_thread_cont(void *v, wait_result_t wres)
{
	struct ifnet *ifp = v;
	struct ifclassq *ifq = &ifp->if_snd;

	lck_mtx_lock_spin(&ifp->if_start_lock);
	if (__improbable(wres == THREAD_INTERRUPTED ||
	    ifp->if_start_thread == THREAD_NULL)) {
		goto terminate;
	}

	if (__improbable(ifp->if_start_embryonic)) {
		ifp->if_start_embryonic = 0;
		lck_mtx_unlock(&ifp->if_start_lock);
		ifnet_decr_pending_thread_count(ifp);
		lck_mtx_lock_spin(&ifp->if_start_lock);
		goto skip;
	}

	ifp->if_start_active = 1;

	  
                                            
    
	for (;;) {
		u_int32_t req = ifp->if_start_req;
		if (!IFCQ_IS_EMPTY(ifq) &&
		    (ifp->if_eflags & IFEF_ENQUEUE_MULTI) &&
		    ifp->if_start_delayed == 0 &&
		    IFCQ_LEN(ifq) < ifp->if_start_delay_qlen &&
		    (ifp->if_eflags & IFEF_DELAY_START)) {
			ifp->if_start_delayed = 1;
			ifnet_start_delayed++;
			break;
		} else {
			ifp->if_start_delayed = 0;
		}
		lck_mtx_unlock(&ifp->if_start_lock);

		  
                                                        
                                                  
                                                       
                     
     
		if (!ifnet_datamov_begin(ifp)) {
			lck_mtx_lock_spin(&ifp->if_start_lock);
			break;
		}

		                                       
		((*ifp->if_start)(ifp));

		  
                                          
     
		ifnet_datamov_end(ifp);

		lck_mtx_lock_spin(&ifp->if_start_lock);

		  
                                            
                                             
     
		if (req == ifp->if_start_req ||
		    (ifp->if_start_flags & IFSF_FLOW_CONTROLLED)) {
			break;
		}
	}
skip:
	ifp->if_start_req = 0;
	ifp->if_start_active = 0;


	if (__probable(ifp->if_start_thread != THREAD_NULL)) {
		uint64_t deadline = TIMEOUT_WAIT_FOREVER;
		struct timespec delay_start_ts;
		struct timespec *ts;

		  
                                                           
                                                            
                                                              
                                         
     
		ts = ((IFCQ_TBR_IS_ENABLED(ifq) && !IFCQ_IS_EMPTY(ifq)) ?
		    &ifp->if_start_cycle : NULL);

		if (ts == NULL && ifp->if_start_delayed == 1) {
			delay_start_ts.tv_sec = 0;
			delay_start_ts.tv_nsec = ifp->if_start_delay_timeout;
			ts = &delay_start_ts;
		}

		if (ts != NULL && ts->tv_sec == 0 && ts->tv_nsec == 0) {
			ts = NULL;
		}

		if (__improbable(ts != NULL)) {
			clock_interval_to_deadline((uint32_t)(ts->tv_nsec +
			    (ts->tv_sec * NSEC_PER_SEC)), 1, &deadline);
		}

		(void) assert_wait_deadline(&ifp->if_start_thread,
		    THREAD_UNINT, deadline);
		lck_mtx_unlock(&ifp->if_start_lock);
		(void) thread_block_parameter(ifnet_start_thread_cont, ifp);
		                
	} else {
terminate:
		                            
		ifnet_set_start_cycle(ifp, NULL);
		lck_mtx_unlock(&ifp->if_start_lock);
		ifnet_purge(ifp);

		if (dlil_verbose) {
			DLIL_PRINTF("%s: starter thread terminated\n",
			    if_name(ifp));
		}

		                                                     
		thread_deallocate(current_thread());
		                     
		thread_terminate(current_thread());
		                
	}

	                         
	VERIFY(0);
	                
	__builtin_unreachable();
}

void
ifnet_set_start_cycle(struct ifnet *ifp, struct timespec *ts)
{
	if (ts == NULL) {
		bzero(&ifp->if_start_cycle, sizeof(ifp->if_start_cycle));
	} else {
		*(&ifp->if_start_cycle) = *ts;
	}

	if (ts != NULL && ts->tv_nsec != 0 && dlil_verbose) {
		DLIL_PRINTF("%s: restart interval set to %lu nsec\n",
		    if_name(ifp), ts->tv_nsec);
	}
}

static inline void
ifnet_poll_wakeup(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(&ifp->if_poll_lock, LCK_MTX_ASSERT_OWNED);

	ifp->if_poll_req++;
	if (!(ifp->if_poll_flags & IF_POLLF_RUNNING) &&
	    ifp->if_poll_thread != THREAD_NULL) {
		wakeup_one((caddr_t)&ifp->if_poll_thread);
	}
}

void
ifnet_poll(struct ifnet *ifp)
{
	  
                                                           
    
	lck_mtx_lock_spin(&ifp->if_poll_lock);
	ifnet_poll_wakeup(ifp);
	lck_mtx_unlock(&ifp->if_poll_lock);
}

__attribute__((noreturn))
static void
ifnet_poll_thread_func(void *v, wait_result_t w)
{
#pragma unused(w)
	char thread_name[MAXTHREADNAMESIZE];
	struct ifnet *ifp = v;

	VERIFY(ifp->if_eflags & IFEF_RXPOLL);
	VERIFY(current_thread() == ifp->if_poll_thread);

	                                                           
	bzero(thread_name, sizeof(thread_name));
	(void) snprintf(thread_name, sizeof(thread_name),
	    "ifnet_poller_%s", ifp->if_xname);
	thread_set_thread_name(ifp->if_poll_thread, thread_name);

	lck_mtx_lock(&ifp->if_poll_lock);
	VERIFY(!(ifp->if_poll_flags & (IF_POLLF_EMBRYONIC | IF_POLLF_RUNNING)));
	(void) assert_wait(&ifp->if_poll_thread, THREAD_UNINT);
	ifp->if_poll_flags |= IF_POLLF_EMBRYONIC;
	                                                
	ifnet_poll_wakeup(ifp);
	lck_mtx_unlock(&ifp->if_poll_lock);
	(void) thread_block_parameter(ifnet_poll_thread_cont, ifp);
	                
	__builtin_unreachable();
}

__attribute__((noreturn))
static void
ifnet_poll_thread_cont(void *v, wait_result_t wres)
{
	struct dlil_threading_info *inp;
	struct ifnet *ifp = v;
	struct ifnet_stat_increment_param s;
	struct timespec start_time;

	VERIFY(ifp->if_eflags & IFEF_RXPOLL);

	bzero(&s, sizeof(s));
	net_timerclear(&start_time);

	lck_mtx_lock_spin(&ifp->if_poll_lock);
	if (__improbable(wres == THREAD_INTERRUPTED ||
	    ifp->if_poll_thread == THREAD_NULL)) {
		goto terminate;
	}

	inp = ifp->if_inp;
	VERIFY(inp != NULL);

	if (__improbable(ifp->if_poll_flags & IF_POLLF_EMBRYONIC)) {
		ifp->if_poll_flags &= ~IF_POLLF_EMBRYONIC;
		lck_mtx_unlock(&ifp->if_poll_lock);
		ifnet_decr_pending_thread_count(ifp);
		lck_mtx_lock_spin(&ifp->if_poll_lock);
		goto skip;
	}

	ifp->if_poll_flags |= IF_POLLF_RUNNING;

	  
                                            
    
	for (;;) {
		struct mbuf *m_head, *m_tail;
		u_int32_t m_lim, m_cnt, m_totlen;
		u_int16_t req = ifp->if_poll_req;

		m_lim = (ifp->if_rxpoll_plim != 0) ? ifp->if_rxpoll_plim :
		    MAX((qlimit(&inp->dlth_pkts)), (ifp->if_rxpoll_phiwat << 2));
		lck_mtx_unlock(&ifp->if_poll_lock);

		  
                                                  
                                                    
                                                  
     
		if (!ifnet_is_attached(ifp, 1)) {
			lck_mtx_lock_spin(&ifp->if_poll_lock);
			break;
		}

		if (dlil_verbose > 1) {
			DLIL_PRINTF("%s: polling up to %d pkts, "
			    "pkts avg %d max %d, wreq avg %d, "
			    "bytes avg %d\n",
			    if_name(ifp), m_lim,
			    ifp->if_rxpoll_pavg, ifp->if_rxpoll_pmax,
			    ifp->if_rxpoll_wavg, ifp->if_rxpoll_bavg);
		}

		                                            
		((*ifp->if_input_poll)(ifp, 0, m_lim, &m_head, &m_tail,
		&m_cnt, &m_totlen));

		if (m_head != NULL) {
			VERIFY(m_tail != NULL && m_cnt > 0);

			if (dlil_verbose > 1) {
				DLIL_PRINTF("%s: polled %d pkts, "
				    "pkts avg %d max %d, wreq avg %d, "
				    "bytes avg %d\n",
				    if_name(ifp), m_cnt,
				    ifp->if_rxpoll_pavg, ifp->if_rxpoll_pmax,
				    ifp->if_rxpoll_wavg, ifp->if_rxpoll_bavg);
			}

			                                             
			s.packets_in = m_cnt;
			s.bytes_in = m_totlen;

			(void) ifnet_input_common(ifp, m_head, m_tail,
			    &s, TRUE, TRUE);
		} else {
			if (dlil_verbose > 1) {
				DLIL_PRINTF("%s: no packets, "
				    "pkts avg %d max %d, wreq avg %d, "
				    "bytes avg %d\n",
				    if_name(ifp), ifp->if_rxpoll_pavg,
				    ifp->if_rxpoll_pmax, ifp->if_rxpoll_wavg,
				    ifp->if_rxpoll_bavg);
			}

			(void) ifnet_input_common(ifp, NULL, NULL,
			    NULL, FALSE, TRUE);
		}

		                              
		ifnet_decr_iorefcnt(ifp);

		lck_mtx_lock_spin(&ifp->if_poll_lock);

		                                               
		if (req == ifp->if_poll_req ||
		    ifp->if_poll_thread == THREAD_NULL) {
			break;
		}
	}
skip:
	ifp->if_poll_req = 0;
	ifp->if_poll_flags &= ~IF_POLLF_RUNNING;

	if (ifp->if_poll_thread != THREAD_NULL) {
		uint64_t deadline = TIMEOUT_WAIT_FOREVER;
		struct timespec *ts;

		  
                                                              
                                        
     
		ts = &ifp->if_poll_cycle;
		if (ts->tv_sec == 0 && ts->tv_nsec == 0) {
			ts = NULL;
		}

		if (ts != NULL) {
			clock_interval_to_deadline((uint32_t)(ts->tv_nsec +
			    (ts->tv_sec * NSEC_PER_SEC)), 1, &deadline);
		}

		(void) assert_wait_deadline(&ifp->if_poll_thread,
		    THREAD_UNINT, deadline);
		lck_mtx_unlock(&ifp->if_poll_lock);
		(void) thread_block_parameter(ifnet_poll_thread_cont, ifp);
		                
	} else {
terminate:
		                                                 
		ifnet_set_poll_cycle(ifp, NULL);
		lck_mtx_unlock(&ifp->if_poll_lock);

		if (dlil_verbose) {
			DLIL_PRINTF("%s: poller thread terminated\n",
			    if_name(ifp));
		}

		                                                     
		thread_deallocate(current_thread());
		                     
		thread_terminate(current_thread());
		                
	}

	                         
	VERIFY(0);
	                
	__builtin_unreachable();
}

void
ifnet_set_poll_cycle(struct ifnet *ifp, struct timespec *ts)
{
	if (ts == NULL) {
		bzero(&ifp->if_poll_cycle, sizeof(ifp->if_poll_cycle));
	} else {
		*(&ifp->if_poll_cycle) = *ts;
	}

	if (ts != NULL && ts->tv_nsec != 0 && dlil_verbose) {
		DLIL_PRINTF("%s: poll interval set to %lu nsec\n",
		    if_name(ifp), ts->tv_nsec);
	}
}

void
ifnet_purge(struct ifnet *ifp)
{
	if (ifp != NULL && (ifp->if_eflags & IFEF_TXSTART)) {
		if_qflush(ifp, 0);
	}
}

void
ifnet_update_sndq(struct ifclassq *ifq, cqev_t ev)
{
	IFCQ_LOCK_ASSERT_HELD(ifq);

	if (!(IFCQ_IS_READY(ifq))) {
		return;
	}

	if (IFCQ_TBR_IS_ENABLED(ifq)) {
		struct tb_profile tb = {
			.rate = ifq->ifcq_tbr.tbr_rate_raw,
			.percent = ifq->ifcq_tbr.tbr_percent, .depth = 0
		};
		(void) ifclassq_tbr_set(ifq, &tb, FALSE);
	}

	ifclassq_update(ifq, ev);
}

void
ifnet_update_rcv(struct ifnet *ifp, cqev_t ev)
{
	switch (ev) {
	case CLASSQ_EV_LINK_BANDWIDTH:
		if (net_rxpoll && (ifp->if_eflags & IFEF_RXPOLL)) {
			ifp->if_poll_update++;
		}
		break;

	default:
		break;
	}
}

errno_t
ifnet_set_output_sched_model(struct ifnet *ifp, u_int32_t model)
{
	struct ifclassq *ifq;
	u_int32_t omodel;
	errno_t err;

	if (ifp == NULL || model >= IFNET_SCHED_MODEL_MAX) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return ENXIO;
	}

	ifq = &ifp->if_snd;
	IFCQ_LOCK(ifq);
	omodel = ifp->if_output_sched_model;
	ifp->if_output_sched_model = model;
	if ((err = ifclassq_pktsched_setup(ifq)) != 0) {
		ifp->if_output_sched_model = omodel;
	}
	IFCQ_UNLOCK(ifq);

	return err;
}

errno_t
ifnet_set_sndq_maxlen(struct ifnet *ifp, u_int32_t maxqlen)
{
	if (ifp == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return ENXIO;
	}

	ifclassq_set_maxlen(&ifp->if_snd, maxqlen);

	return 0;
}

errno_t
ifnet_get_sndq_maxlen(struct ifnet *ifp, u_int32_t *maxqlen)
{
	if (ifp == NULL || maxqlen == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return ENXIO;
	}

	*maxqlen = ifclassq_get_maxlen(&ifp->if_snd);

	return 0;
}

errno_t
ifnet_get_sndq_len(struct ifnet *ifp, u_int32_t *pkts)
{
	errno_t err;

	if (ifp == NULL || pkts == NULL) {
		err = EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART)) {
		err = ENXIO;
	} else {
		err = ifclassq_get_len(&ifp->if_snd, MBUF_SC_UNSPEC,
		    pkts, NULL);
	}

	return err;
}

errno_t
ifnet_get_service_class_sndq_len(struct ifnet *ifp, mbuf_svc_class_t sc,
    u_int32_t *pkts, u_int32_t *bytes)
{
	errno_t err;

	if (ifp == NULL || !MBUF_VALID_SC(sc) ||
	    (pkts == NULL && bytes == NULL)) {
		err = EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART)) {
		err = ENXIO;
	} else {
		err = ifclassq_get_len(&ifp->if_snd, sc, pkts, bytes);
	}

	return err;
}

errno_t
ifnet_set_rcvq_maxlen(struct ifnet *ifp, u_int32_t maxqlen)
{
	struct dlil_threading_info *inp;

	if (ifp == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_RXPOLL) || ifp->if_inp == NULL) {
		return ENXIO;
	}

	if (maxqlen == 0) {
		maxqlen = if_rcvq_maxlen;
	} else if (maxqlen < IF_RCVQ_MINLEN) {
		maxqlen = IF_RCVQ_MINLEN;
	}

	inp = ifp->if_inp;
	lck_mtx_lock(&inp->dlth_lock);
	qlimit(&inp->dlth_pkts) = maxqlen;
	lck_mtx_unlock(&inp->dlth_lock);

	return 0;
}

errno_t
ifnet_get_rcvq_maxlen(struct ifnet *ifp, u_int32_t *maxqlen)
{
	struct dlil_threading_info *inp;

	if (ifp == NULL || maxqlen == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_RXPOLL) || ifp->if_inp == NULL) {
		return ENXIO;
	}

	inp = ifp->if_inp;
	lck_mtx_lock(&inp->dlth_lock);
	*maxqlen = qlimit(&inp->dlth_pkts);
	lck_mtx_unlock(&inp->dlth_lock);
	return 0;
}

void
ifnet_enqueue_multi_setup(struct ifnet *ifp, uint16_t delay_qlen,
    uint16_t delay_timeout)
{
	if (delay_qlen > 0 && delay_timeout > 0) {
		if_set_eflags(ifp, IFEF_ENQUEUE_MULTI);
		ifp->if_start_delay_qlen = MIN(100, delay_qlen);
		ifp->if_start_delay_timeout = min(20000, delay_timeout);
		                                    
		ifp->if_start_delay_timeout *= 1000;
		kprintf("%s: forced IFEF_ENQUEUE_MULTI qlen %u timeout %u\n",
		    ifp->if_xname, (uint32_t)delay_qlen,
		    (uint32_t)delay_timeout);
	} else {
		if_clear_eflags(ifp, IFEF_ENQUEUE_MULTI);
	}
}

  
                                                                              
                                                                              
                             
   
static __attribute__((noinline)) void
ifnet_mcast_clear_dscp(uint8_t *buf, uint8_t ip_ver)
{
	struct ip *ip;
	struct ip6_hdr *ip6;
	uint8_t lbuf[64] __attribute__((aligned(8)));
	uint8_t *p = buf;

	if (ip_ver == IPVERSION) {
		uint8_t old_tos;
		uint32_t sum;

		if (__improbable(!IP_HDR_ALIGNED_P(p))) {
			DTRACE_IP1(not__aligned__v4, uint8_t *, buf);
			bcopy(buf, lbuf, sizeof(struct ip));
			p = lbuf;
		}
		ip = (struct ip *)(void *)p;
		if (__probable((ip->ip_tos & ~IPTOS_ECN_MASK) == 0)) {
			return;
		}

		DTRACE_IP1(clear__v4, struct ip *, ip);
		old_tos = ip->ip_tos;
		ip->ip_tos &= IPTOS_ECN_MASK;
		sum = ip->ip_sum + htons(old_tos) - htons(ip->ip_tos);
		sum = (sum >> 16) + (sum & 0xffff);
		ip->ip_sum = (uint16_t)(sum & 0xffff);

		if (__improbable(p == lbuf)) {
			bcopy(lbuf, buf, sizeof(struct ip));
		}
	} else {
		uint32_t flow;
		ASSERT(ip_ver == IPV6_VERSION);

		if (__improbable(!IP_HDR_ALIGNED_P(p))) {
			DTRACE_IP1(not__aligned__v6, uint8_t *, buf);
			bcopy(buf, lbuf, sizeof(struct ip6_hdr));
			p = lbuf;
		}
		ip6 = (struct ip6_hdr *)(void *)p;
		flow = ntohl(ip6->ip6_flow);
		if (__probable((flow & IP6FLOW_DSCP_MASK) == 0)) {
			return;
		}

		DTRACE_IP1(clear__v6, struct ip6_hdr *, ip6);
		ip6->ip6_flow = htonl(flow & ~IP6FLOW_DSCP_MASK);

		if (__improbable(p == lbuf)) {
			bcopy(lbuf, buf, sizeof(struct ip6_hdr));
		}
	}
}

static inline errno_t
ifnet_enqueue_ifclassq(struct ifnet *ifp, classq_pkt_t *p, boolean_t flush,
    boolean_t *pdrop)
{
	volatile uint64_t *fg_ts = NULL;
	volatile uint64_t *rt_ts = NULL;
	struct timespec now;
	u_int64_t now_nsec = 0;
	int error = 0;
	uint8_t *mcast_buf = NULL;
	uint8_t ip_ver;
	uint32_t pktlen;

	ASSERT(ifp->if_eflags & IFEF_TXSTART);

	  
                                                                    
                                                                  
                                                                  
                                                 
    
	switch (p->cp_ptype) {
	case QP_MBUF:
		ASSERT(p->cp_mbuf->m_flags & M_PKTHDR);
		ASSERT(p->cp_mbuf->m_nextpkt == NULL);

		if (!(p->cp_mbuf->m_pkthdr.pkt_flags & PKTF_TS_VALID) ||
		    p->cp_mbuf->m_pkthdr.pkt_timestamp == 0) {
			nanouptime(&now);
			net_timernsec(&now, &now_nsec);
			p->cp_mbuf->m_pkthdr.pkt_timestamp = now_nsec;
		}
		p->cp_mbuf->m_pkthdr.pkt_flags &= ~PKTF_TS_VALID;
		  
                                                   
                                                     
                            
     
		if ((p->cp_mbuf->m_pkthdr.pkt_flags & PKTF_FLOW_ID) &&
		    p->cp_mbuf->m_pkthdr.pkt_flowsrc == FLOWSRC_INPCB) {
			if (!(p->cp_mbuf->m_pkthdr.pkt_flags &
			    PKTF_SO_BACKGROUND)) {
				ifp->if_fg_sendts = (uint32_t)_net_uptime;
				if (fg_ts != NULL) {
					*fg_ts = (uint32_t)_net_uptime;
				}
			}
			if (p->cp_mbuf->m_pkthdr.pkt_flags & PKTF_SO_REALTIME) {
				ifp->if_rt_sendts = (uint32_t)_net_uptime;
				if (rt_ts != NULL) {
					*rt_ts = (uint32_t)_net_uptime;
				}
			}
		}
		pktlen = m_pktlen(p->cp_mbuf);

		  
                                                          
                                                              
                                                            
                             
     
		if ((p->cp_mbuf->m_flags & M_MCAST) != 0 &&
		    IFNET_IS_WIFI_INFRA(ifp)) {
			size_t len = mbuf_len(p->cp_mbuf), hlen;
			struct ether_header *eh;
			boolean_t pullup = FALSE;
			uint16_t etype;

			if (__improbable(len < sizeof(struct ether_header))) {
				DTRACE_IP1(small__ether, size_t, len);
				if ((p->cp_mbuf = m_pullup(p->cp_mbuf,
				    sizeof(struct ether_header))) == NULL) {
					return ENOMEM;
				}
			}
			eh = (struct ether_header *)mbuf_data(p->cp_mbuf);
			etype = ntohs(eh->ether_type);
			if (etype == ETHERTYPE_IP) {
				hlen = sizeof(struct ether_header) +
				    sizeof(struct ip);
				if (len < hlen) {
					DTRACE_IP1(small__v4, size_t, len);
					pullup = TRUE;
				}
				ip_ver = IPVERSION;
			} else if (etype == ETHERTYPE_IPV6) {
				hlen = sizeof(struct ether_header) +
				    sizeof(struct ip6_hdr);
				if (len < hlen) {
					DTRACE_IP1(small__v6, size_t, len);
					pullup = TRUE;
				}
				ip_ver = IPV6_VERSION;
			} else {
				DTRACE_IP1(invalid__etype, uint16_t, etype);
				break;
			}
			if (pullup) {
				if ((p->cp_mbuf = m_pullup(p->cp_mbuf, (int)hlen)) ==
				    NULL) {
					return ENOMEM;
				}

				eh = (struct ether_header *)mbuf_data(
					p->cp_mbuf);
			}
			mcast_buf = (uint8_t *)(eh + 1);
			  
                                                          
                                                       
                                 
      
		}
		break;


	default:
		VERIFY(0);
		                
		__builtin_unreachable();
	}

	if (mcast_buf != NULL) {
		ifnet_mcast_clear_dscp(mcast_buf, ip_ver);
	}

	if (ifp->if_eflags & IFEF_ENQUEUE_MULTI) {
		if (now_nsec == 0) {
			nanouptime(&now);
			net_timernsec(&now, &now_nsec);
		}
		  
                                                    
                                                        
                                                     
                                                         
                                                        
                                             
                                                          
                                                          
                                    
                                                      
                                                           
                                       
     
		u_int64_t dwin = (ifp->if_start_delay_timeout << 1);
		if (ifp->if_start_delay_swin > 0) {
			if ((ifp->if_start_delay_swin + dwin) > now_nsec) {
				ifp->if_start_delay_cnt++;
			} else if ((now_nsec - ifp->if_start_delay_swin)
			    >= (200 * 1000 * 1000)) {
				ifp->if_start_delay_swin = now_nsec;
				ifp->if_start_delay_cnt = 1;
				ifp->if_start_delay_idle = 0;
				if (ifp->if_eflags & IFEF_DELAY_START) {
					if_clear_eflags(ifp, IFEF_DELAY_START);
					ifnet_delay_start_disabled_increment();
				}
			} else {
				if (ifp->if_start_delay_cnt >=
				    ifp->if_start_delay_qlen) {
					if_set_eflags(ifp, IFEF_DELAY_START);
					ifp->if_start_delay_idle = 0;
				} else {
					if (ifp->if_start_delay_idle >= 10) {
						if_clear_eflags(ifp,
						    IFEF_DELAY_START);
						ifnet_delay_start_disabled_increment();
					} else {
						ifp->if_start_delay_idle++;
					}
				}
				ifp->if_start_delay_swin = now_nsec;
				ifp->if_start_delay_cnt = 1;
			}
		} else {
			ifp->if_start_delay_swin = now_nsec;
			ifp->if_start_delay_cnt = 1;
			ifp->if_start_delay_idle = 0;
			if_clear_eflags(ifp, IFEF_DELAY_START);
		}
	} else {
		if_clear_eflags(ifp, IFEF_DELAY_START);
	}

	                                                 
	error = ifclassq_enqueue(&ifp->if_snd, p, p, 1, pktlen, pdrop);

	  
                                                                    
                                                                        
                                                
    
	if (!(ifp->if_eflags & IFEF_ENQUEUE_MULTI) &&
	    ((error == 0 && flush) || error == EQFULL || error == EQSUSPENDED)) {
		ifnet_start(ifp);
	}

	return error;
}

static inline errno_t
ifnet_enqueue_ifclassq_chain(struct ifnet *ifp, classq_pkt_t *head,
    classq_pkt_t *tail, uint32_t cnt, uint32_t bytes, boolean_t flush,
    boolean_t *pdrop)
{
	int error;

	                                                 
	error = ifclassq_enqueue(&ifp->if_snd, head, tail, cnt, bytes, pdrop);

	  
                                                                    
                                                                        
                                                
    
	if ((error == 0 && flush) || error == EQFULL || error == EQSUSPENDED) {
		ifnet_start(ifp);
	}
	return error;
}

int
ifnet_enqueue_netem(void *handle, pktsched_pkt_t *pkts, uint32_t n_pkts)
{
	struct ifnet *ifp = handle;
	boolean_t pdrop;                   
	uint32_t i;

	ASSERT(n_pkts >= 1);
	for (i = 0; i < n_pkts - 1; i++) {
		(void) ifnet_enqueue_ifclassq(ifp, &pkts[i].pktsched_pkt,
		    FALSE, &pdrop);
	}
	                                
	(void) ifnet_enqueue_ifclassq(ifp, &pkts[i].pktsched_pkt, TRUE, &pdrop);

	return 0;
}

static inline errno_t
ifnet_enqueue_common(struct ifnet *ifp, classq_pkt_t *pkt, boolean_t flush,
    boolean_t *pdrop)
{
	if (ifp->if_output_netem != NULL) {
		return netem_enqueue(ifp->if_output_netem, pkt, pdrop);
	} else {
		return ifnet_enqueue_ifclassq(ifp, pkt, flush, pdrop);
	}
}

errno_t
ifnet_enqueue(struct ifnet *ifp, struct mbuf *m)
{
	boolean_t pdrop;
	return ifnet_enqueue_mbuf(ifp, m, TRUE, &pdrop);
}

errno_t
ifnet_enqueue_mbuf(struct ifnet *ifp, struct mbuf *m, boolean_t flush,
    boolean_t *pdrop)
{
	classq_pkt_t pkt;

	if (ifp == NULL || m == NULL || !(m->m_flags & M_PKTHDR) ||
	    m->m_nextpkt != NULL) {
		if (m != NULL) {
			m_freem_list(m);
			*pdrop = TRUE;
		}
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    !IF_FULLY_ATTACHED(ifp)) {
		                                              
		m_freem(m);
		*pdrop = TRUE;
		return ENXIO;
	} else if (!(ifp->if_flags & IFF_UP)) {
		m_freem(m);
		*pdrop = TRUE;
		return ENETDOWN;
	}

	CLASSQ_PKT_INIT_MBUF(&pkt, m);
	return ifnet_enqueue_common(ifp, &pkt, flush, pdrop);
}

errno_t
ifnet_enqueue_mbuf_chain(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, uint32_t cnt, uint32_t bytes, boolean_t flush,
    boolean_t *pdrop)
{
	classq_pkt_t head, tail;

	ASSERT(m_head != NULL);
	ASSERT((m_head->m_flags & M_PKTHDR) != 0);
	ASSERT(m_tail != NULL);
	ASSERT((m_tail->m_flags & M_PKTHDR) != 0);
	ASSERT(ifp != NULL);
	ASSERT((ifp->if_eflags & IFEF_TXSTART) != 0);

	if (!IF_FULLY_ATTACHED(ifp)) {
		                                              
		m_freem_list(m_head);
		*pdrop = TRUE;
		return ENXIO;
	} else if (!(ifp->if_flags & IFF_UP)) {
		m_freem_list(m_head);
		*pdrop = TRUE;
		return ENETDOWN;
	}

	CLASSQ_PKT_INIT_MBUF(&head, m_head);
	CLASSQ_PKT_INIT_MBUF(&tail, m_tail);
	return ifnet_enqueue_ifclassq_chain(ifp, &head, &tail, cnt, bytes,
	           flush, pdrop);
}


errno_t
ifnet_dequeue(struct ifnet *ifp, struct mbuf **mp)
{
	errno_t rc;
	classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);

	if (ifp == NULL || mp == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    ifp->if_output_sched_model >= IFNET_SCHED_MODEL_MAX) {
		return ENXIO;
	}
	if (!ifnet_is_attached(ifp, 1)) {
		return ENXIO;
	}

	rc = ifclassq_dequeue(&ifp->if_snd, 1, CLASSQ_DEQUEUE_MAX_BYTE_LIMIT,
	    &pkt, NULL, NULL, NULL);
	VERIFY((pkt.cp_ptype == QP_MBUF) || (pkt.cp_mbuf == NULL));
	ifnet_decr_iorefcnt(ifp);
	*mp = pkt.cp_mbuf;
	return rc;
}

errno_t
ifnet_dequeue_service_class(struct ifnet *ifp, mbuf_svc_class_t sc,
    struct mbuf **mp)
{
	errno_t rc;
	classq_pkt_t pkt = CLASSQ_PKT_INITIALIZER(pkt);

	if (ifp == NULL || mp == NULL || !MBUF_VALID_SC(sc)) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    ifp->if_output_sched_model >= IFNET_SCHED_MODEL_MAX) {
		return ENXIO;
	}
	if (!ifnet_is_attached(ifp, 1)) {
		return ENXIO;
	}

	rc = ifclassq_dequeue_sc(&ifp->if_snd, sc, 1,
	    CLASSQ_DEQUEUE_MAX_BYTE_LIMIT, &pkt, NULL, NULL, NULL);
	VERIFY((pkt.cp_ptype == QP_MBUF) || (pkt.cp_mbuf == NULL));
	ifnet_decr_iorefcnt(ifp);
	*mp = pkt.cp_mbuf;
	return rc;
}

errno_t
ifnet_dequeue_multi(struct ifnet *ifp, u_int32_t pkt_limit,
    struct mbuf **head, struct mbuf **tail, u_int32_t *cnt, u_int32_t *len)
{
	errno_t rc;
	classq_pkt_t pkt_head = CLASSQ_PKT_INITIALIZER(pkt_head);
	classq_pkt_t pkt_tail = CLASSQ_PKT_INITIALIZER(pkt_tail);

	if (ifp == NULL || head == NULL || pkt_limit < 1) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    ifp->if_output_sched_model >= IFNET_SCHED_MODEL_MAX) {
		return ENXIO;
	}
	if (!ifnet_is_attached(ifp, 1)) {
		return ENXIO;
	}

	rc = ifclassq_dequeue(&ifp->if_snd, pkt_limit,
	    CLASSQ_DEQUEUE_MAX_BYTE_LIMIT, &pkt_head, &pkt_tail, cnt, len);
	VERIFY((pkt_head.cp_ptype == QP_MBUF) || (pkt_head.cp_mbuf == NULL));
	ifnet_decr_iorefcnt(ifp);
	*head = pkt_head.cp_mbuf;
	if (tail != NULL) {
		*tail = pkt_tail.cp_mbuf;
	}
	return rc;
}

errno_t
ifnet_dequeue_multi_bytes(struct ifnet *ifp, u_int32_t byte_limit,
    struct mbuf **head, struct mbuf **tail, u_int32_t *cnt, u_int32_t *len)
{
	errno_t rc;
	classq_pkt_t pkt_head = CLASSQ_PKT_INITIALIZER(pkt_head);
	classq_pkt_t pkt_tail = CLASSQ_PKT_INITIALIZER(pkt_tail);

	if (ifp == NULL || head == NULL || byte_limit < 1) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    ifp->if_output_sched_model >= IFNET_SCHED_MODEL_MAX) {
		return ENXIO;
	}
	if (!ifnet_is_attached(ifp, 1)) {
		return ENXIO;
	}

	rc = ifclassq_dequeue(&ifp->if_snd, CLASSQ_DEQUEUE_MAX_PKT_LIMIT,
	    byte_limit, &pkt_head, &pkt_tail, cnt, len);
	VERIFY((pkt_head.cp_ptype == QP_MBUF) || (pkt_head.cp_mbuf == NULL));
	ifnet_decr_iorefcnt(ifp);
	*head = pkt_head.cp_mbuf;
	if (tail != NULL) {
		*tail = pkt_tail.cp_mbuf;
	}
	return rc;
}

errno_t
ifnet_dequeue_service_class_multi(struct ifnet *ifp, mbuf_svc_class_t sc,
    u_int32_t pkt_limit, struct mbuf **head, struct mbuf **tail, u_int32_t *cnt,
    u_int32_t *len)
{
	errno_t rc;
	classq_pkt_t pkt_head = CLASSQ_PKT_INITIALIZER(pkt_head);
	classq_pkt_t pkt_tail = CLASSQ_PKT_INITIALIZER(pkt_tail);

	if (ifp == NULL || head == NULL || pkt_limit < 1 ||
	    !MBUF_VALID_SC(sc)) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    ifp->if_output_sched_model >= IFNET_SCHED_MODEL_MAX) {
		return ENXIO;
	}
	if (!ifnet_is_attached(ifp, 1)) {
		return ENXIO;
	}

	rc = ifclassq_dequeue_sc(&ifp->if_snd, sc, pkt_limit,
	    CLASSQ_DEQUEUE_MAX_BYTE_LIMIT, &pkt_head, &pkt_tail,
	    cnt, len);
	VERIFY((pkt_head.cp_ptype == QP_MBUF) || (pkt_head.cp_mbuf == NULL));
	ifnet_decr_iorefcnt(ifp);
	*head = pkt_head.cp_mbuf;
	if (tail != NULL) {
		*tail = pkt_tail.cp_mbuf;
	}
	return rc;
}

#if XNU_TARGET_OS_OSX
errno_t
ifnet_framer_stub(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *dest, const char *dest_linkaddr,
    const char *frame_type, u_int32_t *pre, u_int32_t *post)
{
	if (pre != NULL) {
		*pre = 0;
	}
	if (post != NULL) {
		*post = 0;
	}

	return ifp->if_framer_legacy(ifp, m, dest, dest_linkaddr, frame_type);
}
#endif                        

static boolean_t
packet_has_vlan_tag(struct mbuf * m)
{
	u_int   tag = 0;

	if ((m->m_pkthdr.csum_flags & CSUM_VLAN_TAG_VALID) != 0) {
		tag = EVL_VLANOFTAG(m->m_pkthdr.vlan_tag);
		if (tag == 0) {
			                                                       
			m->m_pkthdr.csum_flags &= ~CSUM_VLAN_TAG_VALID;
		}
	}
	return tag != 0;
}

static int
dlil_interface_filters_input(struct ifnet *ifp, struct mbuf **m_p,
    char **frame_header_p, protocol_family_t protocol_family)
{
	boolean_t               is_vlan_packet = FALSE;
	struct ifnet_filter     *filter;
	struct mbuf             *m = *m_p;

	is_vlan_packet = packet_has_vlan_tag(m);

	if (TAILQ_EMPTY(&ifp->if_flt_head)) {
		return 0;
	}

	  
                                                    
    
	lck_mtx_lock_spin(&ifp->if_flt_lock);
	                                                                
	if_flt_monitor_busy(ifp);
	TAILQ_FOREACH(filter, &ifp->if_flt_head, filt_next) {
		int result;

		                                                           
		if (is_vlan_packet &&
		    (filter->filt_flags & DLIL_IFF_INTERNAL) == 0) {
			continue;
		}

		if (!filter->filt_skip && filter->filt_input != NULL &&
		    (filter->filt_protocol == 0 ||
		    filter->filt_protocol == protocol_family)) {
			lck_mtx_unlock(&ifp->if_flt_lock);

			result = (*filter->filt_input)(filter->filt_cookie,
			    ifp, protocol_family, m_p, frame_header_p);

			lck_mtx_lock_spin(&ifp->if_flt_lock);
			if (result != 0) {
				                                     
				if_flt_monitor_unbusy(ifp);
				lck_mtx_unlock(&ifp->if_flt_lock);
				return result;
			}
		}
	}
	                                     
	if_flt_monitor_unbusy(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	  
                                                                   
                                                                    
    
	if (*m_p != NULL) {
		(*m_p)->m_flags &= ~M_PROTO1;
	}

	return 0;
}

static int
dlil_interface_filters_output(struct ifnet *ifp, struct mbuf **m_p,
    protocol_family_t protocol_family)
{
	boolean_t               is_vlan_packet;
	struct ifnet_filter     *filter;
	struct mbuf             *m = *m_p;

	is_vlan_packet = packet_has_vlan_tag(m);

	  
                                                     
    
	lck_mtx_lock_spin(&ifp->if_flt_lock);
	                                                                
	if_flt_monitor_busy(ifp);
	TAILQ_FOREACH(filter, &ifp->if_flt_head, filt_next) {
		int result;

		                                                           
		if (is_vlan_packet &&
		    (filter->filt_flags & DLIL_IFF_INTERNAL) == 0) {
			continue;
		}

		if (!filter->filt_skip && filter->filt_output != NULL &&
		    (filter->filt_protocol == 0 ||
		    filter->filt_protocol == protocol_family)) {
			lck_mtx_unlock(&ifp->if_flt_lock);

			result = filter->filt_output(filter->filt_cookie, ifp,
			    protocol_family, m_p);

			lck_mtx_lock_spin(&ifp->if_flt_lock);
			if (result != 0) {
				                                     
				if_flt_monitor_unbusy(ifp);
				lck_mtx_unlock(&ifp->if_flt_lock);
				return result;
			}
		}
	}
	                                     
	if_flt_monitor_unbusy(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	return 0;
}

static void
dlil_ifproto_input(struct if_proto * ifproto, mbuf_t m)
{
	int error;

	if (ifproto->proto_kpi == kProtoKPI_v1) {
		                                                  
		while (m != NULL) {
			char *  frame_header;
			mbuf_t  next_packet;

			next_packet = m->m_nextpkt;
			m->m_nextpkt = NULL;
			frame_header = m->m_pkthdr.pkt_hdr;
			m->m_pkthdr.pkt_hdr = NULL;
			error = (*ifproto->kpi.v1.input)(ifproto->ifp,
			    ifproto->protocol_family, m, frame_header);
			if (error != 0 && error != EJUSTRETURN) {
				m_freem(m);
			}
			m = next_packet;
		}
	} else if (ifproto->proto_kpi == kProtoKPI_v2) {
		                                              
		error = (*ifproto->kpi.v2.input)(ifproto->ifp,
		    ifproto->protocol_family, m);
		if (error != 0 && error != EJUSTRETURN) {
			m_freem_list(m);
		}
	}
}

static void
dlil_input_stats_add(const struct ifnet_stat_increment_param *s,
    struct dlil_threading_info *inp, struct ifnet *ifp, boolean_t poll)
{
	struct ifnet_stat_increment_param *d = &inp->dlth_stats;

	if (s->packets_in != 0) {
		d->packets_in += s->packets_in;
	}
	if (s->bytes_in != 0) {
		d->bytes_in += s->bytes_in;
	}
	if (s->errors_in != 0) {
		d->errors_in += s->errors_in;
	}

	if (s->packets_out != 0) {
		d->packets_out += s->packets_out;
	}
	if (s->bytes_out != 0) {
		d->bytes_out += s->bytes_out;
	}
	if (s->errors_out != 0) {
		d->errors_out += s->errors_out;
	}

	if (s->collisions != 0) {
		d->collisions += s->collisions;
	}
	if (s->dropped != 0) {
		d->dropped += s->dropped;
	}

	if (poll) {
		PKTCNTR_ADD(&ifp->if_poll_tstats, s->packets_in, s->bytes_in);
	}
}

static boolean_t
dlil_input_stats_sync(struct ifnet *ifp, struct dlil_threading_info *inp)
{
	struct ifnet_stat_increment_param *s = &inp->dlth_stats;

	  
                                                        
                                                           
    
	if (s->packets_in != 0) {
		atomic_add_64(&ifp->if_data.ifi_ipackets, s->packets_in);
		s->packets_in = 0;
	}
	if (s->bytes_in != 0) {
		atomic_add_64(&ifp->if_data.ifi_ibytes, s->bytes_in);
		s->bytes_in = 0;
	}
	if (s->errors_in != 0) {
		atomic_add_64(&ifp->if_data.ifi_ierrors, s->errors_in);
		s->errors_in = 0;
	}

	if (s->packets_out != 0) {
		atomic_add_64(&ifp->if_data.ifi_opackets, s->packets_out);
		s->packets_out = 0;
	}
	if (s->bytes_out != 0) {
		atomic_add_64(&ifp->if_data.ifi_obytes, s->bytes_out);
		s->bytes_out = 0;
	}
	if (s->errors_out != 0) {
		atomic_add_64(&ifp->if_data.ifi_oerrors, s->errors_out);
		s->errors_out = 0;
	}

	if (s->collisions != 0) {
		atomic_add_64(&ifp->if_data.ifi_collisions, s->collisions);
		s->collisions = 0;
	}
	if (s->dropped != 0) {
		atomic_add_64(&ifp->if_data.ifi_iqdrops, s->dropped);
		s->dropped = 0;
	}

	  
                                                           
                                                   
    
	if (ifp->if_poll_tstats.packets != 0) {
		ifp->if_poll_pstats.ifi_poll_packets += ifp->if_poll_tstats.packets;
		ifp->if_poll_tstats.packets = 0;
	}
	if (ifp->if_poll_tstats.bytes != 0) {
		ifp->if_poll_pstats.ifi_poll_bytes += ifp->if_poll_tstats.bytes;
		ifp->if_poll_tstats.bytes = 0;
	}

	return ifp->if_data_threshold != 0;
}

__private_extern__ void
dlil_input_packet_list(struct ifnet *ifp, struct mbuf *m)
{
	return dlil_input_packet_list_common(ifp, m, 0,
	           IFNET_MODEL_INPUT_POLL_OFF, FALSE);
}

__private_extern__ void
dlil_input_packet_list_extended(struct ifnet *ifp, struct mbuf *m,
    u_int32_t cnt, ifnet_model_t mode)
{
	return dlil_input_packet_list_common(ifp, m, cnt, mode, TRUE);
}

static void
dlil_input_packet_list_common(struct ifnet *ifp_param, struct mbuf *m,
    u_int32_t cnt, ifnet_model_t mode, boolean_t ext)
{
	int error = 0;
	protocol_family_t protocol_family;
	mbuf_t next_packet;
	ifnet_t ifp = ifp_param;
	char *frame_header = NULL;
	struct if_proto *last_ifproto = NULL;
	mbuf_t pkt_first = NULL;
	mbuf_t *pkt_next = NULL;
	u_int32_t poll_thresh = 0, poll_ival = 0;
	int iorefcnt = 0;

	KERNEL_DEBUG(DBG_FNC_DLIL_INPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	if (ext && mode == IFNET_MODEL_INPUT_POLL_ON && cnt > 1 &&
	    (poll_ival = if_rxpoll_interval_pkts) > 0) {
		poll_thresh = cnt;
	}

	while (m != NULL) {
		struct if_proto *ifproto = NULL;
		uint32_t pktf_mask;                                

		if (ifp_param == NULL) {
			ifp = m->m_pkthdr.rcvif;
		}

		if ((ifp->if_eflags & IFEF_RXPOLL) &&
		    (ifp->if_xflags & IFXF_LEGACY) && poll_thresh != 0 &&
		    poll_ival > 0 && (--poll_thresh % poll_ival) == 0) {
			ifnet_poll(ifp);
		}

		                                    
		MBUF_INPUT_CHECK(m, ifp);

		next_packet = m->m_nextpkt;
		m->m_nextpkt = NULL;
		frame_header = m->m_pkthdr.pkt_hdr;
		m->m_pkthdr.pkt_hdr = NULL;

		  
                                                      
                                                      
                                
     
		if (ifp != lo_ifp) {
			                                               
			if (iorefcnt == 0) {
				if (!ifnet_datamov_begin(ifp)) {
					m_freem(m);
					goto next;
				}
			}
			iorefcnt = 1;
			  
                                                   
      
			pktf_mask = PKTF_TS_VALID | PKTF_SKIP_PKTAP;
		} else {
			  
                                                     
                                                     
                                    
      
			pktf_mask = (PKTF_LOOP | PKTF_IFAINFO);
		}

		                                     
		m_classifier_init(m, pktf_mask);

		ifp_inc_traffic_class_in(ifp, m);

		                                                   
		ifnet_lock_shared(ifp);
		error = (*ifp->if_demux)(ifp, m, frame_header,
		    &protocol_family);
		ifnet_lock_done(ifp);
		if (error != 0) {
			if (error == EJUSTRETURN) {
				goto next;
			}
			protocol_family = 0;
		}

		pktap_input(ifp, protocol_family, m, frame_header);

		                                                          
		if (protocol_family == PF_INET && IS_INTF_CLAT46(ifp)) {
			m_freem(m);
			ip6stat.ip6s_clat464_in_v4_drop++;
			goto next;
		}

		                                                              
		if (protocol_family == PF_INET6 && IS_INTF_CLAT46(ifp)
		    && dlil_is_clat_needed(protocol_family, m)) {
			char *data = NULL;
			struct ether_header eh;
			struct ether_header *ehp = NULL;

			if (ifp->if_type == IFT_ETHER) {
				ehp = (struct ether_header *)(void *)frame_header;
				                                                   
				if (ntohs(ehp->ether_type) != ETHERTYPE_IPV6) {
					goto skip_clat;
				}

				                                                      
				bcopy(frame_header, (caddr_t)&eh, ETHER_HDR_LEN);
			}
			error = dlil_clat64(ifp, &protocol_family, &m);
			data = (char *) mbuf_data(m);
			if (error != 0) {
				m_freem(m);
				ip6stat.ip6s_clat464_in_drop++;
				goto next;
			}
			                               
			if (protocol_family != PF_INET) {
				goto skip_clat;
			}

			                                             
			switch (ifp->if_type) {
			case IFT_CELLULAR:
				frame_header = data;
				break;
			case IFT_ETHER:
				  
                                           
                                
       
				if (M_LEADINGSPACE(m) < ETHER_HDR_LEN) {
					m_free(m);
					ip6stat.ip6s_clat464_in_drop++;
					goto next;
				}
				  
                                               
                                          
                          
       
				frame_header = data - ETHER_HDR_LEN;
				eh.ether_type = htons(ETHERTYPE_IP);
				bcopy((caddr_t)&eh, frame_header, ETHER_HDR_LEN);
				break;
			}
		}
skip_clat:
		if (hwcksum_dbg != 0 && !(ifp->if_flags & IFF_LOOPBACK) &&
		    !(m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
			dlil_input_cksum_dbg(ifp, m, frame_header,
			    protocol_family);
		}
		  
                                                          
                                                          
                                                       
                                                          
                                                            
    
                                                    
                                                           
                                                       
                                      
     
		if ((m->m_pkthdr.csum_flags & CSUM_ADJUST_DONE) != 0) {
			                                      
		} else if ((m->m_pkthdr.csum_flags &
		    (CSUM_DATA_VALID | CSUM_PARTIAL)) ==
		    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
			int adj;
			if (frame_header == NULL ||
			    frame_header < (char *)mbuf_datastart(m) ||
			    frame_header > (char *)m->m_data ||
			    (adj = (int)(m->m_data - frame_header)) >
			    m->m_pkthdr.csum_rx_start) {
				m->m_pkthdr.csum_data = 0;
				m->m_pkthdr.csum_flags &= ~CSUM_DATA_VALID;
				hwcksum_in_invalidated++;
			} else {
				m->m_pkthdr.csum_rx_start -= adj;
			}
			                                              
			m->m_pkthdr.csum_flags |= CSUM_ADJUST_DONE;
		}
		if (clat_debug) {
			pktap_input(ifp, protocol_family, m, frame_header);
		}

		if (m->m_flags & (M_BCAST | M_MCAST)) {
			atomic_add_64(&ifp->if_imcasts, 1);
		}

		                           
		error = dlil_interface_filters_input(ifp, &m,
		    &frame_header, protocol_family);
		if (error != 0) {
			if (error != EJUSTRETURN) {
				m_freem(m);
			}
			goto next;
		}
		  
                                                               
                                                            
                                                            
                                                          
                                                            
                                            
                                                                 
                                                             
                
     
		if (protocol_family != PF_VLAN &&
		    (m->m_flags & M_PROMISC) != 0) {
			m_freem(m);
			goto next;
		}

		                                                      
		if (protocol_family == 0) {
			ifproto = NULL;
		} else if (last_ifproto != NULL && last_ifproto->ifp == ifp &&
		    (last_ifproto->protocol_family == protocol_family)) {
			VERIFY(ifproto == NULL);
			ifproto = last_ifproto;
			if_proto_ref(last_ifproto);
		} else {
			VERIFY(ifproto == NULL);
			ifnet_lock_shared(ifp);
			                                              
			ifproto = find_attached_proto(ifp, protocol_family);
			ifnet_lock_done(ifp);
		}
		if (ifproto == NULL) {
			                                          
			m_freem(m);
			goto next;
		}
		if (ifproto != last_ifproto) {
			if (last_ifproto != NULL) {
				                                                
				dlil_ifproto_input(last_ifproto, pkt_first);
				pkt_first = NULL;
				if_proto_free(last_ifproto);
			}
			last_ifproto = ifproto;
			if_proto_ref(ifproto);
		}
		                     
		m->m_pkthdr.pkt_hdr = frame_header;
		if (pkt_first == NULL) {
			pkt_first = m;
		} else {
			*pkt_next = m;
		}
		pkt_next = &m->m_nextpkt;

next:
		if (next_packet == NULL && last_ifproto != NULL) {
			                                      
			dlil_ifproto_input(last_ifproto, pkt_first);
			if_proto_free(last_ifproto);
			last_ifproto = NULL;
		}
		if (ifproto != NULL) {
			if_proto_free(ifproto);
			ifproto = NULL;
		}

		m = next_packet;

		                                                     
		if (ifp->if_updatemcasts > 0 && if_mcasts_update(ifp) == 0) {
			ifp->if_updatemcasts = 0;
		}
		if (iorefcnt == 1) {
			                                                                   
			if (!m || (ifp != ifp_param && ifp != m->m_pkthdr.rcvif)) {
				ifnet_datamov_end(ifp);
				iorefcnt = 0;
			}
		}
	}

	KERNEL_DEBUG(DBG_FNC_DLIL_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
}

errno_t
if_mcasts_update(struct ifnet *ifp)
{
	errno_t err;

	err = ifnet_ioctl(ifp, 0, SIOCADDMULTI, NULL);
	if (err == EAFNOSUPPORT) {
		err = 0;
	}
	DLIL_PRINTF("%s: %s %d suspended link-layer multicast membership(s) "
	    "(err=%d)\n", if_name(ifp),
	    (err == 0 ? "successfully restored" : "failed to restore"),
	    ifp->if_updatemcasts, err);

	                         
	return 0;
}

                                                                       
int
dlil_post_complete_msg(struct ifnet *ifp, struct kev_msg *event)
{
	if (ifp != NULL) {
		ifnet_increment_generation(ifp);
	}

#if NECP
	necp_update_all_clients();
#endif           

	return kev_post_msg(event);
}

__private_extern__ void
dlil_post_sifflags_msg(struct ifnet * ifp)
{
	struct kev_msg ev_msg;
	struct net_event_data ev_data;

	bzero(&ev_data, sizeof(ev_data));
	bzero(&ev_msg, sizeof(ev_msg));
	ev_msg.vendor_code = KEV_VENDOR_APPLE;
	ev_msg.kev_class = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass = KEV_DL_SUBCLASS;
	ev_msg.event_code = KEV_DL_SIFFLAGS;
	strlcpy(&ev_data.if_name[0], ifp->if_name, IFNAMSIZ);
	ev_data.if_family = ifp->if_family;
	ev_data.if_unit = (u_int32_t) ifp->if_unit;
	ev_msg.dv[0].data_length = sizeof(struct net_event_data);
	ev_msg.dv[0].data_ptr = &ev_data;
	ev_msg.dv[1].data_length = 0;
	dlil_post_complete_msg(ifp, &ev_msg);
}

#define TMP_IF_PROTO_ARR_SIZE   10
static int
dlil_event_internal(struct ifnet *ifp, struct kev_msg *event, bool update_generation)
{
	struct ifnet_filter *filter = NULL;
	struct if_proto *proto = NULL;
	int if_proto_count = 0;
	struct if_proto **tmp_ifproto_arr = NULL;
	struct if_proto *tmp_ifproto_stack_arr[TMP_IF_PROTO_ARR_SIZE] = {NULL};
	int tmp_ifproto_arr_idx = 0;
	bool tmp_malloc = false;

	  
                                           
    
	lck_mtx_lock_spin(&ifp->if_flt_lock);
	                                                                
	if_flt_monitor_busy(ifp);
	TAILQ_FOREACH(filter, &ifp->if_flt_head, filt_next) {
		if (filter->filt_event != NULL) {
			lck_mtx_unlock(&ifp->if_flt_lock);

			filter->filt_event(filter->filt_cookie, ifp,
			    filter->filt_protocol, event);

			lck_mtx_lock_spin(&ifp->if_flt_lock);
		}
	}
	                                     
	if_flt_monitor_unbusy(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	                                                      
	if (!ifnet_is_attached(ifp, 1)) {
		goto done;
	}

	  
                                                        
                                                              
                                                     
    
	ifnet_lock_shared(ifp);
	if_proto_count = dlil_ifp_protolist(ifp, NULL, 0);
	if (if_proto_count) {
		int i;
		VERIFY(ifp->if_proto_hash != NULL);
		if (if_proto_count <= TMP_IF_PROTO_ARR_SIZE) {
			tmp_ifproto_arr = tmp_ifproto_stack_arr;
		} else {
			MALLOC(tmp_ifproto_arr, struct if_proto **,
			    sizeof(*tmp_ifproto_arr) * if_proto_count,
			    M_TEMP, M_ZERO);
			if (tmp_ifproto_arr == NULL) {
				ifnet_lock_done(ifp);
				goto cleanup;
			}
			tmp_malloc = true;
		}

		for (i = 0; i < PROTO_HASH_SLOTS; i++) {
			SLIST_FOREACH(proto, &ifp->if_proto_hash[i],
			    next_hash) {
				if_proto_ref(proto);
				tmp_ifproto_arr[tmp_ifproto_arr_idx] = proto;
				tmp_ifproto_arr_idx++;
			}
		}
		VERIFY(if_proto_count == tmp_ifproto_arr_idx);
	}
	ifnet_lock_done(ifp);

	for (tmp_ifproto_arr_idx = 0; tmp_ifproto_arr_idx < if_proto_count;
	    tmp_ifproto_arr_idx++) {
		proto = tmp_ifproto_arr[tmp_ifproto_arr_idx];
		VERIFY(proto != NULL);
		proto_media_event eventp =
		    (proto->proto_kpi == kProtoKPI_v1 ?
		    proto->kpi.v1.event :
		    proto->kpi.v2.event);

		if (eventp != NULL) {
			eventp(ifp, proto->protocol_family,
			    event);
		}
		if_proto_free(proto);
	}

cleanup:
	if (tmp_malloc) {
		FREE(tmp_ifproto_arr, M_TEMP);
	}

	                                     
	if (ifp->if_event != NULL) {
		ifp->if_event(ifp, event);
	}

	                              
	ifnet_decr_iorefcnt(ifp);
done:
	return dlil_post_complete_msg(update_generation ? ifp : NULL, event);
}

errno_t
ifnet_event(ifnet_t ifp, struct kern_event_msg *event)
{
	struct kev_msg kev_msg;
	int result = 0;

	if (ifp == NULL || event == NULL) {
		return EINVAL;
	}

	bzero(&kev_msg, sizeof(kev_msg));
	kev_msg.vendor_code = event->vendor_code;
	kev_msg.kev_class = event->kev_class;
	kev_msg.kev_subclass = event->kev_subclass;
	kev_msg.event_code = event->event_code;
	kev_msg.dv[0].data_ptr = &event->event_data[0];
	kev_msg.dv[0].data_length = event->total_size - KEV_MSG_HEADER_SIZE;
	kev_msg.dv[1].data_length = 0;

	result = dlil_event_internal(ifp, &kev_msg, TRUE);

	return result;
}

static void
dlil_count_chain_len(mbuf_t m, struct chain_len_stats *cls)
{
	mbuf_t  n = m;
	int chainlen = 0;

	while (n != NULL) {
		chainlen++;
		n = n->m_next;
	}
	switch (chainlen) {
	case 0:
		break;
	case 1:
		atomic_add_64(&cls->cls_one, 1);
		break;
	case 2:
		atomic_add_64(&cls->cls_two, 1);
		break;
	case 3:
		atomic_add_64(&cls->cls_three, 1);
		break;
	case 4:
		atomic_add_64(&cls->cls_four, 1);
		break;
	case 5:
	default:
		atomic_add_64(&cls->cls_five_or_more, 1);
		break;
	}
}

  
              
  
                                                                   
                                                                 
                                                                 
                                      
  
                                                                  
                                                                  
                                                                
                              
  
                                                             
                                                                
               
   
errno_t
dlil_output(ifnet_t ifp, protocol_family_t proto_family, mbuf_t packetlist,
    void *route, const struct sockaddr *dest, int raw, struct flowadv *adv)
{
	char *frame_type = NULL;
	char *dst_linkaddr = NULL;
	int retval = 0;
	char frame_type_buffer[MAX_FRAME_TYPE_SIZE * 4];
	char dst_linkaddr_buffer[MAX_LINKADDR * 4];
	struct if_proto *proto = NULL;
	mbuf_t  m = NULL;
	mbuf_t  send_head = NULL;
	mbuf_t  *send_tail = &send_head;
	int iorefcnt = 0;
	u_int32_t pre = 0, post = 0;
	u_int32_t fpkts = 0, fbytes = 0;
	int32_t flen = 0;
	struct timespec now;
	u_int64_t now_nsec;
	boolean_t did_clat46 = FALSE;
	protocol_family_t old_proto_family = proto_family;
	struct sockaddr_in6 dest6;
	struct rtentry *rt = NULL;
	u_int32_t m_loop_set = 0;

	KERNEL_DEBUG(DBG_FNC_DLIL_OUTPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	  
                                                                         
                                                      
    
	if (!ifnet_datamov_begin(ifp)) {
		retval = ENXIO;
		goto cleanup;
	}
	iorefcnt = 1;

	VERIFY(ifp->if_output_dlil != NULL);

	                                                     
	if (ifp->if_updatemcasts > 0 && if_mcasts_update(ifp) == 0) {
		ifp->if_updatemcasts = 0;
	}

	frame_type = frame_type_buffer;
	dst_linkaddr = dst_linkaddr_buffer;

	if (raw == 0) {
		ifnet_lock_shared(ifp);
		                                              
		proto = find_attached_proto(ifp, proto_family);
		if (proto == NULL) {
			ifnet_lock_done(ifp);
			retval = ENXIO;
			goto cleanup;
		}
		ifnet_lock_done(ifp);
	}

preout_again:
	if (packetlist == NULL) {
		goto cleanup;
	}

	m = packetlist;
	packetlist = packetlist->m_nextpkt;
	m->m_nextpkt = NULL;

	  
                                                    
                                                       
                                           
    
	if (proto_family == PF_INET && IS_INTF_CLAT46(ifp) &&
	    (ifp->if_type == IFT_CELLULAR ||
	    dlil_is_clat_needed(proto_family, m))) {
		retval = dlil_clat46(ifp, &proto_family, &m);
		  
                                               
     
		if (retval != 0) {
			m_freem(m);
			m = NULL;
			ip6stat.ip6s_clat464_out_drop++;
			                                                
			ASSERT(proto_family == PF_INET);
			goto preout_again;
		}
		  
                                                                    
    
                                                         
                                          
     
		if (!did_clat46 && proto_family == PF_INET6) {
			did_clat46 = TRUE;

			if (proto != NULL) {
				if_proto_free(proto);
			}
			ifnet_lock_shared(ifp);
			                                              
			proto = find_attached_proto(ifp, proto_family);
			if (proto == NULL) {
				ifnet_lock_done(ifp);
				retval = ENXIO;
				m_freem(m);
				m = NULL;
				goto cleanup;
			}
			ifnet_lock_done(ifp);
			if (ifp->if_type == IFT_ETHER) {
				                                              
				dest6.sin6_len = sizeof(struct sockaddr_in6);
				dest6.sin6_family = AF_INET6;
				dest6.sin6_addr = (mtod(m, struct ip6_hdr *))->ip6_dst;
				dest = (const struct sockaddr *)&dest6;

				  
                                                 
                                         
       
				rt = rtalloc1_scoped((struct sockaddr *)&dest6,
				    0, 0, ifp->if_index);

				route = rt;
			}
		}
	}

	  
                                                              
                                                                  
                                                                  
                                             
   
                                                                  
                                                                   
                                  
   
                                                                  
                                                                
                     
   
                                                                   
                                 
    
	if (raw == 0) {
		proto_media_preout preoutp = (proto->proto_kpi == kProtoKPI_v1 ?
		    proto->kpi.v1.pre_output : proto->kpi.v2.pre_output);
		retval = 0;
		if (preoutp != NULL) {
			retval = preoutp(ifp, proto_family, &m, dest, route,
			    frame_type, dst_linkaddr);

			if (retval != 0) {
				if (retval == EJUSTRETURN) {
					goto preout_again;
				}
				m_freem(m);
				m = NULL;
				goto cleanup;
			}
		}
	}

	do {
		  
                                                    
                                                      
                                                          
                                                       
     
		if ((raw != 0) && (ifp->if_family == IFNET_FAMILY_ETHERNET)) {
			uint8_t vlan_encap_len = 0;

			if ((m->m_pkthdr.csum_flags & CSUM_VLAN_ENCAP_PRESENT) != 0) {
				vlan_encap_len = ETHER_VLAN_ENCAP_LEN;
			}
			m->m_pkthdr.pkt_hdr = mtod(m, char *) + ETHER_HDR_LEN + vlan_encap_len;
		} else {
			m->m_pkthdr.pkt_hdr = mtod(m, void *);
		}

		  
                                                  
                                                         
                     
    
                                                             
                                       
    
                                                             
                                         
     
		if (proto_family == PF_INET && IS_INTF_CLAT46(ifp) &&
		    (ifp->if_type == IFT_CELLULAR ||
		    dlil_is_clat_needed(proto_family, m))) {
			retval = dlil_clat46(ifp, &proto_family, &m);
			                                                   
			if (retval != 0) {
				m_freem(m);
				m = NULL;
				ip6stat.ip6s_clat464_out_drop++;
				goto next;
			}
		}

#if CONFIG_DTRACE
		if (!raw && proto_family == PF_INET) {
			struct ip *ip = mtod(m, struct ip *);
			DTRACE_IP6(send, struct mbuf *, m, struct inpcb *, NULL,
			    struct ip *, ip, struct ifnet *, ifp,
			    struct ip *, ip, struct ip6_hdr *, NULL);
		} else if (!raw && proto_family == PF_INET6) {
			struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
			DTRACE_IP6(send, struct mbuf *, m, struct inpcb *, NULL,
			    struct ip6_hdr *, ip6, struct ifnet *, ifp,
			    struct ip *, NULL, struct ip6_hdr *, ip6);
		}
#endif                    

		if (raw == 0 && ifp->if_framer != NULL) {
			int rcvif_set = 0;

			  
                                                    
                                                      
                                                   
                                                    
                                                   
                                   
      
			if ((m->m_flags & (M_BCAST | M_LOOP)) &&
			    m->m_pkthdr.rcvif == NULL) {
				m->m_pkthdr.rcvif = ifp;
				rcvif_set = 1;
			}
			m_loop_set = m->m_flags & M_LOOP;
			retval = ifp->if_framer(ifp, &m, dest, dst_linkaddr,
			    frame_type, &pre, &post);
			if (retval != 0) {
				if (retval != EJUSTRETURN) {
					m_freem(m);
				}
				goto next;
			}

			  
                                                    
                                                      
      
			if ((m->m_pkthdr.csum_flags &
			    (CSUM_DATA_VALID | CSUM_PARTIAL)) ==
			    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
				m->m_pkthdr.csum_tx_stuff += pre;
				m->m_pkthdr.csum_tx_start += pre;
			}

			if (hwcksum_dbg != 0 && !(ifp->if_flags & IFF_LOOPBACK)) {
				dlil_output_cksum_dbg(ifp, m, pre,
				    proto_family);
			}

			  
                                                  
                                               
                                                 
                                                    
                                                   
                                                   
                  
      
			if (rcvif_set && m->m_pkthdr.rcvif == ifp) {
				m->m_pkthdr.rcvif = NULL;
			}
		}

		  
                                                      
     
		retval = dlil_interface_filters_output(ifp, &m, proto_family);
		if (retval != 0) {
			if (retval != EJUSTRETURN) {
				m_freem(m);
			}
			goto next;
		}
		  
                                                    
                                                          
     
		m->m_flags &= ~M_PROTO1;

		  
                                                             
                                                               
                                                             
                                                                
                                                             
                                                 
     
		if (!(ifp->if_hwassist & IFNET_MULTIPAGES)) {
			if ((m = m_normalize(m)) == NULL) {
				goto next;
			}
		}

		  
                                                           
                              
     
		if (TSO_IPV4_NOTOK(ifp, m) || TSO_IPV6_NOTOK(ifp, m)) {
			retval = EMSGSIZE;
			m_freem(m);
			goto cleanup;
		}

		ifp_inc_traffic_class_out(ifp, m);

		pktap_output(ifp, proto_family, m, pre, post);

		  
                                                   
     
		if (tx_chain_len_count) {
			dlil_count_chain_len(m, &tx_chain_len_stats);
		}

		  
                                                         
                                                         
                                                          
                                                       
                    
     
		nanouptime(&now);
		net_timernsec(&now, &now_nsec);
		(void) mbuf_set_timestamp(m, now_nsec, TRUE);

		  
                                                              
                                                              
                                                  
     
		if ((m->m_pkthdr.pkt_flags & PKTF_FORWARDED) &&
		    (m->m_pkthdr.csum_flags & (CSUM_DATA_VALID | CSUM_PARTIAL)) ==
		    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
			m->m_pkthdr.csum_flags &= ~CSUM_TX_FLAGS;
			m->m_pkthdr.csum_data = 0;
		}

		  
                              
     
		if (ifp->if_eflags & (IFEF_SENDLIST | IFEF_ENQUEUE_MULTI)) {
			if (m->m_pkthdr.pkt_flags & PKTF_FORWARDED) {
				flen += (m_pktlen(m) - (pre + post));
				m->m_pkthdr.pkt_flags &= ~PKTF_FORWARDED;
			}
			*send_tail = m;
			send_tail = &m->m_nextpkt;
		} else {
			if (m->m_pkthdr.pkt_flags & PKTF_FORWARDED) {
				flen = (m_pktlen(m) - (pre + post));
				m->m_pkthdr.pkt_flags &= ~PKTF_FORWARDED;
			} else {
				flen = 0;
			}
			KERNEL_DEBUG(DBG_FNC_DLIL_IFOUT | DBG_FUNC_START,
			    0, 0, 0, 0, 0);
			retval = (*ifp->if_output_dlil)(ifp, m);
			if (retval == EQFULL || retval == EQSUSPENDED) {
				if (adv != NULL && adv->code == FADV_SUCCESS) {
					adv->code = (retval == EQFULL ?
					    FADV_FLOW_CONTROLLED :
					    FADV_SUSPENDED);
				}
				retval = 0;
			}
			if (retval == 0 && flen > 0) {
				fbytes += flen;
				fpkts++;
			}
			if (retval != 0 && dlil_verbose) {
				DLIL_PRINTF("%s: output error on %s retval = %d\n",
				    __func__, if_name(ifp),
				    retval);
			}
			KERNEL_DEBUG(DBG_FNC_DLIL_IFOUT | DBG_FUNC_END,
			    0, 0, 0, 0, 0);
		}
		KERNEL_DEBUG(DBG_FNC_DLIL_IFOUT | DBG_FUNC_END, 0, 0, 0, 0, 0);

next:
		m = packetlist;
		if (m != NULL) {
			m->m_flags |= m_loop_set;
			packetlist = packetlist->m_nextpkt;
			m->m_nextpkt = NULL;
		}
		                                                         
		if (did_clat46) {
			proto_family = old_proto_family;
		}
	} while (m != NULL);

	if (send_head != NULL) {
		KERNEL_DEBUG(DBG_FNC_DLIL_IFOUT | DBG_FUNC_START,
		    0, 0, 0, 0, 0);
		if (ifp->if_eflags & IFEF_SENDLIST) {
			retval = (*ifp->if_output_dlil)(ifp, send_head);
			if (retval == EQFULL || retval == EQSUSPENDED) {
				if (adv != NULL) {
					adv->code = (retval == EQFULL ?
					    FADV_FLOW_CONTROLLED :
					    FADV_SUSPENDED);
				}
				retval = 0;
			}
			if (retval == 0 && flen > 0) {
				fbytes += flen;
				fpkts++;
			}
			if (retval != 0 && dlil_verbose) {
				DLIL_PRINTF("%s: output error on %s retval = %d\n",
				    __func__, if_name(ifp), retval);
			}
		} else {
			struct mbuf *send_m;
			int enq_cnt = 0;
			VERIFY(ifp->if_eflags & IFEF_ENQUEUE_MULTI);
			while (send_head != NULL) {
				send_m = send_head;
				send_head = send_m->m_nextpkt;
				send_m->m_nextpkt = NULL;
				retval = (*ifp->if_output_dlil)(ifp, send_m);
				if (retval == EQFULL || retval == EQSUSPENDED) {
					if (adv != NULL) {
						adv->code = (retval == EQFULL ?
						    FADV_FLOW_CONTROLLED :
						    FADV_SUSPENDED);
					}
					retval = 0;
				}
				if (retval == 0) {
					enq_cnt++;
					if (flen > 0) {
						fpkts++;
					}
				}
				if (retval != 0 && dlil_verbose) {
					DLIL_PRINTF("%s: output error on %s "
					    "retval = %d\n",
					    __func__, if_name(ifp), retval);
				}
			}
			if (enq_cnt > 0) {
				fbytes += flen;
				ifnet_start(ifp);
			}
		}
		KERNEL_DEBUG(DBG_FNC_DLIL_IFOUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	}

	KERNEL_DEBUG(DBG_FNC_DLIL_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);

cleanup:
	if (fbytes > 0) {
		ifp->if_fbytes += fbytes;
	}
	if (fpkts > 0) {
		ifp->if_fpackets += fpkts;
	}
	if (proto != NULL) {
		if_proto_free(proto);
	}
	if (packetlist) {                                        
		mbuf_freem_list(packetlist);
	}
	if (retval == EJUSTRETURN) {
		retval = 0;
	}
	if (iorefcnt == 1) {
		ifnet_datamov_end(ifp);
	}
	if (rt != NULL) {
		rtfree(rt);
		rt = NULL;
	}

	return retval;
}

  
                                                                                
                                  
   
static int
dlil_is_clat_needed(protocol_family_t proto_family, mbuf_t m)
{
	int ret = 0;
	switch (proto_family) {
	case PF_INET: {
		struct ip *iph = mtod(m, struct ip *);
		if (CLAT46_NEEDED(ntohl(iph->ip_dst.s_addr))) {
			ret = 1;
		}
		break;
	}
	case PF_INET6: {
		struct ip6_hdr *ip6h = mtod(m, struct ip6_hdr *);
		if ((size_t)m_pktlen(m) >= sizeof(struct ip6_hdr) &&
		    CLAT64_NEEDED(&ip6h->ip6_dst)) {
			ret = 1;
		}
		break;
	}
	}

	return ret;
}
  
                                                             
                                                                  
                                           
  
                                      
                                                                            
                                             
                                                                              
                                                               
  
                                                 
   
static errno_t
dlil_clat46(ifnet_t ifp, protocol_family_t *proto_family, mbuf_t *m)
{
	VERIFY(*proto_family == PF_INET);
	VERIFY(IS_INTF_CLAT46(ifp));

	pbuf_t pbuf_store, *pbuf = NULL;
	struct ip *iph = NULL;
	struct in_addr osrc, odst;
	uint8_t proto = 0;
	struct in6_ifaddr *ia6_clat_src = NULL;
	struct in6_addr *src = NULL;
	struct in6_addr dst;
	int error = 0;
	uint16_t off = 0;
	uint16_t tot_len = 0;
	uint16_t ip_id_val = 0;
	uint16_t ip_frag_off = 0;

	boolean_t is_frag = FALSE;
	boolean_t is_first_frag = TRUE;
	boolean_t is_last_frag = TRUE;

	pbuf_init_mbuf(&pbuf_store, *m, ifp);
	pbuf = &pbuf_store;
	iph = pbuf->pb_data;

	osrc = iph->ip_src;
	odst = iph->ip_dst;
	proto = iph->ip_p;
	off = (uint16_t)(iph->ip_hl << 2);
	ip_id_val = iph->ip_id;
	ip_frag_off = ntohs(iph->ip_off) & IP_OFFMASK;

	tot_len = ntohs(iph->ip_len);

	  
                                        
                                
                                                  
                            
    
	if (ntohs(iph->ip_off) & ~(IP_DF | IP_RF)) {
		is_frag = TRUE;

		                                                     
		if (ip_frag_off != 0) {
			is_first_frag = FALSE;
		}

		                                               
		if (ntohs(iph->ip_off) & IP_MF) {
			is_last_frag = FALSE;
		}
	}

	  
                                                                
                
    
	ia6_clat_src = in6ifa_ifpwithflag(ifp, IN6_IFF_CLAT46);
	if (ia6_clat_src == NULL) {
		ip6stat.ip6s_clat464_out_nov6addr_drop++;
		error = -1;
		goto cleanup;
	}

	src = &ia6_clat_src->ia_addr.sin6_addr;

	  
                                                               
                                                  
    
	if ((error = nat464_synthesize_ipv6(ifp, &odst, &dst)) != 0) {
		ip6stat.ip6s_clat464_out_v6synthfail_drop++;
		goto cleanup;
	}

	                                        
	error = (nat464_translate_46(pbuf, off, iph->ip_tos, iph->ip_p,
	    iph->ip_ttl, *src, dst, tot_len) == NT_NAT64) ? 0 : -1;

	iph = NULL;                                                   

	if (error != 0) {
		ip6stat.ip6s_clat464_out_46transfail_drop++;
		goto cleanup;
	}

	  
                                                              
                       
    
	error = (nat464_translate_proto(pbuf, (struct nat464_addr *)&osrc, (struct nat464_addr *)&odst,
	    proto, PF_INET, PF_INET6, NT_OUT, !is_first_frag) == NT_NAT64) ? 0 : -1;

	if (error != 0) {
		ip6stat.ip6s_clat464_out_46proto_transfail_drop++;
		goto cleanup;
	}

	                                         
	if (is_frag) {
		error = nat464_insert_frag46(pbuf, ip_id_val, ip_frag_off, is_last_frag);

		if (error != 0) {
			ip6stat.ip6s_clat464_out_46frag_transfail_drop++;
			goto cleanup;
		}
	}

cleanup:
	if (ia6_clat_src != NULL) {
		IFA_REMREF(&ia6_clat_src->ia_ifa);
	}

	if (pbuf_is_valid(pbuf)) {
		*m = pbuf->pb_mbuf;
		pbuf->pb_mbuf = NULL;
		pbuf_destroy(pbuf);
	} else {
		error = -1;
		ip6stat.ip6s_clat464_out_invalpbuf_drop++;
	}

	if (error == 0) {
		*proto_family = PF_INET6;
		ip6stat.ip6s_clat464_out_success++;
	}

	return error;
}

  
                                                               
                                                                 
                        
  
                                                 
   
static errno_t
dlil_clat64(ifnet_t ifp, protocol_family_t *proto_family, mbuf_t *m)
{
	VERIFY(*proto_family == PF_INET6);
	VERIFY(IS_INTF_CLAT46(ifp));

	struct ip6_hdr *ip6h = NULL;
	struct in6_addr osrc, odst;
	uint8_t proto = 0;
	struct in6_ifaddr *ia6_clat_dst = NULL;
	struct in_ifaddr *ia4_clat_dst = NULL;
	struct in_addr *dst = NULL;
	struct in_addr src;
	int error = 0;
	uint32_t off = 0;
	u_int64_t tot_len = 0;
	uint8_t tos = 0;
	boolean_t is_first_frag = TRUE;

	                                                     
	if ((size_t)(*m)->m_pkthdr.len < sizeof(struct ip6_hdr) ||
	    ((size_t)(*m)->m_len < sizeof(struct ip6_hdr) &&
	    (*m = m_pullup(*m, sizeof(struct ip6_hdr))) == NULL)) {
		ip6stat.ip6s_clat464_in_tooshort_drop++;
		return -1;
	}

	ip6h = mtod(*m, struct ip6_hdr *);
	                                                               
	if ((size_t)(*m)->m_pkthdr.len < ntohs(ip6h->ip6_plen) + sizeof(struct ip6_hdr)) {
		ip6stat.ip6s_clat464_in_tooshort_drop++;
		return -1;
	}

	osrc = ip6h->ip6_src;
	odst = ip6h->ip6_dst;

	  
                                                    
                                                         
                                                          
    
	ia6_clat_dst = in6ifa_ifpwithflag(ifp, IN6_IFF_CLAT46);
	if (ia6_clat_dst == NULL) {
		goto done;
	}

	  
                                                                    
                       
    
	if (IN6_ARE_ADDR_EQUAL(&odst, &ia6_clat_dst->ia_addr.sin6_addr)) {
		pbuf_t pbuf_store, *pbuf = NULL;
		pbuf_init_mbuf(&pbuf_store, *m, ifp);
		pbuf = &pbuf_store;

		  
                                                                 
                 
     
		ia4_clat_dst = inifa_ifpclatv4(ifp);
		if (ia4_clat_dst == NULL) {
			IFA_REMREF(&ia6_clat_dst->ia_ifa);
			ip6stat.ip6s_clat464_in_nov4addr_drop++;
			error = -1;
			goto cleanup;
		}
		IFA_REMREF(&ia6_clat_dst->ia_ifa);

		                                                                 
		dst = &ia4_clat_dst->ia_addr.sin_addr;
		if ((error = nat464_synthesize_ipv4(ifp, &osrc, &src)) != 0) {
			ip6stat.ip6s_clat464_in_v4synthfail_drop++;
			error = -1;
			goto cleanup;
		}

		ip6h = pbuf->pb_data;
		off = sizeof(struct ip6_hdr);
		proto = ip6h->ip6_nxt;
		tos = (ntohl(ip6h->ip6_flow) >> 20) & 0xff;
		tot_len = ntohs(ip6h->ip6_plen) + sizeof(struct ip6_hdr);

		  
                                                         
                     
     
		error = (nat464_translate_64(pbuf, off, tos, &proto,
		    ip6h->ip6_hlim, src, *dst, tot_len, &is_first_frag) == NT_NAT64) ?
		    0 : -1;

		ip6h = NULL;                                               

		if (error != 0) {
			ip6stat.ip6s_clat464_in_64transfail_drop++;
			goto cleanup;
		}

		  
                                                               
                        
     
		error = (nat464_translate_proto(pbuf, (struct nat464_addr *)&osrc,
		    (struct nat464_addr *)&odst, proto, PF_INET6, PF_INET,
		    NT_IN, !is_first_frag) == NT_NAT64) ? 0 : -1;

		if (error != 0) {
			ip6stat.ip6s_clat464_in_64proto_transfail_drop++;
			goto cleanup;
		}

cleanup:
		if (ia4_clat_dst != NULL) {
			IFA_REMREF(&ia4_clat_dst->ia_ifa);
		}

		if (pbuf_is_valid(pbuf)) {
			*m = pbuf->pb_mbuf;
			pbuf->pb_mbuf = NULL;
			pbuf_destroy(pbuf);
		} else {
			error = -1;
			ip6stat.ip6s_clat464_in_invalpbuf_drop++;
		}

		if (error == 0) {
			*proto_family = PF_INET;
			ip6stat.ip6s_clat464_in_success++;
		}
	}                   

done:
	return error;
}

errno_t
ifnet_ioctl(ifnet_t ifp, protocol_family_t proto_fam, u_long ioctl_code,
    void *ioctl_arg)
{
	struct ifnet_filter *filter;
	int retval = EOPNOTSUPP;
	int result = 0;

	if (ifp == NULL || ioctl_code == 0) {
		return EINVAL;
	}

	                                                      
	if (!ifnet_is_attached(ifp, 1)) {
		return EOPNOTSUPP;
	}

	  
                                    
                                                           
                                   
    
	lck_mtx_lock_spin(&ifp->if_flt_lock);
	                                                                
	if_flt_monitor_busy(ifp);
	TAILQ_FOREACH(filter, &ifp->if_flt_head, filt_next) {
		if (filter->filt_ioctl != NULL && (filter->filt_protocol == 0 ||
		    filter->filt_protocol == proto_fam)) {
			lck_mtx_unlock(&ifp->if_flt_lock);

			result = filter->filt_ioctl(filter->filt_cookie, ifp,
			    proto_fam, ioctl_code, ioctl_arg);

			lck_mtx_lock_spin(&ifp->if_flt_lock);

			                                                        
			if (retval == EOPNOTSUPP || result == EJUSTRETURN) {
				if (result == ENOTSUP) {
					result = EOPNOTSUPP;
				}
				retval = result;
				if (retval != 0 && retval != EOPNOTSUPP) {
					                                     
					if_flt_monitor_unbusy(ifp);
					lck_mtx_unlock(&ifp->if_flt_lock);
					goto cleanup;
				}
			}
		}
	}
	                                     
	if_flt_monitor_unbusy(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	                                            
	if (proto_fam != 0) {
		struct if_proto *proto;

		                                              
		ifnet_lock_shared(ifp);
		proto = find_attached_proto(ifp, proto_fam);
		ifnet_lock_done(ifp);
		if (proto != NULL) {
			proto_media_ioctl ioctlp =
			    (proto->proto_kpi == kProtoKPI_v1 ?
			    proto->kpi.v1.ioctl : proto->kpi.v2.ioctl);
			result = EOPNOTSUPP;
			if (ioctlp != NULL) {
				result = ioctlp(ifp, proto_fam, ioctl_code,
				    ioctl_arg);
			}
			if_proto_free(proto);

			                                                        
			if (retval == EOPNOTSUPP || result == EJUSTRETURN) {
				if (result == ENOTSUP) {
					result = EOPNOTSUPP;
				}
				retval = result;
				if (retval && retval != EOPNOTSUPP) {
					goto cleanup;
				}
			}
		}
	}

	                                      

	  
                                        
                                                      
                                                   
    
	if (ifp->if_ioctl) {
		result = (*ifp->if_ioctl)(ifp, ioctl_code, ioctl_arg);
	}

	                                                        
	if (retval == EOPNOTSUPP || result == EJUSTRETURN) {
		if (result == ENOTSUP) {
			result = EOPNOTSUPP;
		}
		retval = result;
		if (retval && retval != EOPNOTSUPP) {
			goto cleanup;
		}
	}

cleanup:
	if (retval == EJUSTRETURN) {
		retval = 0;
	}

	ifnet_decr_iorefcnt(ifp);

	return retval;
}

__private_extern__ errno_t
dlil_set_bpf_tap(ifnet_t ifp, bpf_tap_mode mode, bpf_packet_func callback)
{
	errno_t error = 0;


	if (ifp->if_set_bpf_tap) {
		                                                            
		if (!ifnet_is_attached(ifp, 1)) {
			return ENXIO;
		}
		error = ifp->if_set_bpf_tap(ifp, mode, callback);
		ifnet_decr_iorefcnt(ifp);
	}
	return error;
}

errno_t
dlil_resolve_multi(struct ifnet *ifp, const struct sockaddr *proto_addr,
    struct sockaddr *ll_addr, size_t ll_len)
{
	errno_t result = EOPNOTSUPP;
	struct if_proto *proto;
	const struct sockaddr *verify;
	proto_media_resolve_multi resolvep;

	if (!ifnet_is_attached(ifp, 1)) {
		return result;
	}

	bzero(ll_addr, ll_len);

	                                                                       
	ifnet_lock_shared(ifp);
	proto = find_attached_proto(ifp, proto_addr->sa_family);
	ifnet_lock_done(ifp);
	if (proto != NULL) {
		resolvep = (proto->proto_kpi == kProtoKPI_v1 ?
		    proto->kpi.v1.resolve_multi : proto->kpi.v2.resolve_multi);
		if (resolvep != NULL) {
			result = resolvep(ifp, proto_addr,
			    (struct sockaddr_dl *)(void *)ll_addr, ll_len);
		}
		if_proto_free(proto);
	}

	                                                    
	if ((result == EOPNOTSUPP || result == 0) && ifp->if_check_multi) {
		if (result == 0) {
			verify = ll_addr;
		} else {
			verify = proto_addr;
		}
		result = ifp->if_check_multi(ifp, verify);
	}

	ifnet_decr_iorefcnt(ifp);
	return result;
}

__private_extern__ errno_t
dlil_send_arp_internal(ifnet_t ifp, u_short arpop,
    const struct sockaddr_dl *sender_hw, const struct sockaddr *sender_proto,
    const struct sockaddr_dl *target_hw, const struct sockaddr *target_proto)
{
	struct if_proto *proto;
	errno_t result = 0;

	                                              
	ifnet_lock_shared(ifp);
	proto = find_attached_proto(ifp, target_proto->sa_family);
	ifnet_lock_done(ifp);
	if (proto == NULL) {
		result = ENOTSUP;
	} else {
		proto_media_send_arp    arpp;
		arpp = (proto->proto_kpi == kProtoKPI_v1 ?
		    proto->kpi.v1.send_arp : proto->kpi.v2.send_arp);
		if (arpp == NULL) {
			result = ENOTSUP;
		} else {
			switch (arpop) {
			case ARPOP_REQUEST:
				arpstat.txrequests++;
				if (target_hw != NULL) {
					arpstat.txurequests++;
				}
				break;
			case ARPOP_REPLY:
				arpstat.txreplies++;
				break;
			}
			result = arpp(ifp, arpop, sender_hw, sender_proto,
			    target_hw, target_proto);
		}
		if_proto_free(proto);
	}

	return result;
}

struct net_thread_marks { };
static const struct net_thread_marks net_thread_marks_base = { };

__private_extern__ const net_thread_marks_t net_thread_marks_none =
    &net_thread_marks_base;

__private_extern__ net_thread_marks_t
net_thread_marks_push(u_int32_t push)
{
	static const char *const base = (const void*)&net_thread_marks_base;
	u_int32_t pop = 0;

	if (push != 0) {
		struct uthread *uth = get_bsdthread_info(current_thread());

		pop = push & ~uth->uu_network_marks;
		if (pop != 0) {
			uth->uu_network_marks |= pop;
		}
	}

	return (net_thread_marks_t)&base[pop];
}

__private_extern__ net_thread_marks_t
net_thread_unmarks_push(u_int32_t unpush)
{
	static const char *const base = (const void*)&net_thread_marks_base;
	u_int32_t unpop = 0;

	if (unpush != 0) {
		struct uthread *uth = get_bsdthread_info(current_thread());

		unpop = unpush & uth->uu_network_marks;
		if (unpop != 0) {
			uth->uu_network_marks &= ~unpop;
		}
	}

	return (net_thread_marks_t)&base[unpop];
}

__private_extern__ void
net_thread_marks_pop(net_thread_marks_t popx)
{
	static const char *const base = (const void*)&net_thread_marks_base;
	const ptrdiff_t pop = (const char *)popx - (const char *)base;

	if (pop != 0) {
		static const ptrdiff_t ones = (ptrdiff_t)(u_int32_t)~0U;
		struct uthread *uth = get_bsdthread_info(current_thread());

		VERIFY((pop & ones) == pop);
		VERIFY((ptrdiff_t)(uth->uu_network_marks & pop) == pop);
		uth->uu_network_marks &= ~pop;
	}
}

__private_extern__ void
net_thread_unmarks_pop(net_thread_marks_t unpopx)
{
	static const char *const base = (const void*)&net_thread_marks_base;
	ptrdiff_t unpop = (const char *)unpopx - (const char *)base;

	if (unpop != 0) {
		static const ptrdiff_t ones = (ptrdiff_t)(u_int32_t)~0U;
		struct uthread *uth = get_bsdthread_info(current_thread());

		VERIFY((unpop & ones) == unpop);
		VERIFY((ptrdiff_t)(uth->uu_network_marks & unpop) == 0);
		uth->uu_network_marks |= unpop;
	}
}

__private_extern__ u_int32_t
net_thread_is_marked(u_int32_t check)
{
	if (check != 0) {
		struct uthread *uth = get_bsdthread_info(current_thread());
		return uth->uu_network_marks & check;
	} else {
		return 0;
	}
}

__private_extern__ u_int32_t
net_thread_is_unmarked(u_int32_t check)
{
	if (check != 0) {
		struct uthread *uth = get_bsdthread_info(current_thread());
		return ~uth->uu_network_marks & check;
	} else {
		return 0;
	}
}

static __inline__ int
_is_announcement(const struct sockaddr_in * sender_sin,
    const struct sockaddr_in * target_sin)
{
	if (target_sin == NULL || sender_sin == NULL) {
		return FALSE;
	}

	return sender_sin->sin_addr.s_addr == target_sin->sin_addr.s_addr;
}

__private_extern__ errno_t
dlil_send_arp(ifnet_t ifp, u_short arpop, const struct sockaddr_dl *sender_hw,
    const struct sockaddr *sender_proto, const struct sockaddr_dl *target_hw,
    const struct sockaddr *target_proto0, u_int32_t rtflags)
{
	errno_t result = 0;
	const struct sockaddr_in * sender_sin;
	const struct sockaddr_in * target_sin;
	struct sockaddr_inarp target_proto_sinarp;
	struct sockaddr *target_proto = (void *)(uintptr_t)target_proto0;

	if (target_proto == NULL || sender_proto == NULL) {
		return EINVAL;
	}

	if (sender_proto->sa_family != target_proto->sa_family) {
		return EINVAL;
	}

	  
                                                     
                                                 
    
	if (rtflags & RTF_ROUTER) {
		bcopy(target_proto, &target_proto_sinarp,
		    sizeof(struct sockaddr_in));
		target_proto_sinarp.sin_other |= SIN_ROUTER;
		target_proto = (struct sockaddr *)&target_proto_sinarp;
	}

	  
                                                          
                                                         
                                                           
              
    
	sender_sin = (struct sockaddr_in *)(void *)(uintptr_t)sender_proto;
	target_sin = (struct sockaddr_in *)(void *)(uintptr_t)target_proto;
	if (target_proto->sa_family == AF_INET &&
	    IN_LINKLOCAL(ntohl(target_sin->sin_addr.s_addr)) &&
	    ipv4_ll_arp_aware != 0 && arpop == ARPOP_REQUEST &&
	    !_is_announcement(sender_sin, target_sin)) {
		ifnet_t         *ifp_list;
		u_int32_t       count;
		u_int32_t       ifp_on;

		result = ENOTSUP;

		if (ifnet_list_get(IFNET_FAMILY_ANY, &ifp_list, &count) == 0) {
			for (ifp_on = 0; ifp_on < count; ifp_on++) {
				errno_t new_result;
				ifaddr_t source_hw = NULL;
				ifaddr_t source_ip = NULL;
				struct sockaddr_in source_ip_copy;
				struct ifnet *cur_ifp = ifp_list[ifp_on];

				  
                                               
                                                  
                                                
       
				if (!(cur_ifp->if_eflags & IFEF_ARPLL)) {
					continue;
				}

				                                
				ifnet_lock_shared(cur_ifp);
				source_hw = cur_ifp->if_lladdr;
				TAILQ_FOREACH(source_ip, &cur_ifp->if_addrhead,
				    ifa_link) {
					IFA_LOCK(source_ip);
					if (source_ip->ifa_addr != NULL &&
					    source_ip->ifa_addr->sa_family ==
					    AF_INET) {
						                                
						source_ip_copy =
						    *(struct sockaddr_in *)
						    (void *)source_ip->ifa_addr;
						IFA_UNLOCK(source_ip);
						break;
					}
					IFA_UNLOCK(source_ip);
				}

				                             
				if (source_ip == NULL) {
					ifnet_lock_done(cur_ifp);
					continue;
				}

				IFA_ADDREF(source_hw);
				ifnet_lock_done(cur_ifp);

				                  
				new_result = dlil_send_arp_internal(cur_ifp,
				    arpop, (struct sockaddr_dl *)(void *)
				    source_hw->ifa_addr,
				    (struct sockaddr *)&source_ip_copy, NULL,
				    target_proto);

				IFA_REMREF(source_hw);
				if (result == ENOTSUP) {
					result = new_result;
				}
			}
			ifnet_list_free(ifp_list);
		}
	} else {
		result = dlil_send_arp_internal(ifp, arpop, sender_hw,
		    sender_proto, target_hw, target_proto);
	}

	return result;
}

  
                                    
   
static int
ifnet_lookup(struct ifnet *ifp)
{
	struct ifnet *_ifp;

	LCK_RW_ASSERT(&ifnet_head_lock, LCK_RW_ASSERT_HELD);
	TAILQ_FOREACH(_ifp, &ifnet_head, if_link) {
		if (_ifp == ifp) {
			break;
		}
	}
	return _ifp != NULL;
}

  
                                                        
                                                          
                                                               
   
int
ifnet_is_attached(struct ifnet *ifp, int refio)
{
	int ret;

	lck_mtx_lock_spin(&ifp->if_ref_lock);
	if ((ret = IF_FULLY_ATTACHED(ifp))) {
		if (refio > 0) {
			ifp->if_refio++;
		}
	}
	lck_mtx_unlock(&ifp->if_ref_lock);

	return ret;
}

void
ifnet_incr_pending_thread_count(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	ifp->if_threads_pending++;
	lck_mtx_unlock(&ifp->if_ref_lock);
}

void
ifnet_decr_pending_thread_count(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	VERIFY(ifp->if_threads_pending > 0);
	ifp->if_threads_pending--;
	if (ifp->if_threads_pending == 0) {
		wakeup(&ifp->if_threads_pending);
	}
	lck_mtx_unlock(&ifp->if_ref_lock);
}

  
                                                                       
                                                                    
                                                                    
   
void
ifnet_incr_iorefcnt(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	VERIFY(IF_FULLY_ATTACHED(ifp));
	VERIFY(ifp->if_refio > 0);
	ifp->if_refio++;
	lck_mtx_unlock(&ifp->if_ref_lock);
}

__attribute__((always_inline))
static void
ifnet_decr_iorefcnt_locked(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(&ifp->if_ref_lock, LCK_MTX_ASSERT_OWNED);

	VERIFY(ifp->if_refio > 0);
	VERIFY(ifp->if_refflags & (IFRF_ATTACHED | IFRF_DETACHING));

	ifp->if_refio--;
	VERIFY(ifp->if_refio != 0 || ifp->if_datamov == 0);

	  
                                                              
                                                 
    
	if (ifp->if_refio == 0 && (ifp->if_refflags & IFRF_DETACHING)) {
		wakeup(&(ifp->if_refio));
	}
}

void
ifnet_decr_iorefcnt(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	ifnet_decr_iorefcnt_locked(ifp);
	lck_mtx_unlock(&ifp->if_ref_lock);
}

boolean_t
ifnet_datamov_begin(struct ifnet *ifp)
{
	boolean_t ret;

	lck_mtx_lock_spin(&ifp->if_ref_lock);
	if ((ret = IF_FULLY_ATTACHED_AND_READY(ifp))) {
		ifp->if_refio++;
		ifp->if_datamov++;
	}
	lck_mtx_unlock(&ifp->if_ref_lock);

	return ret;
}

void
ifnet_datamov_end(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	VERIFY(ifp->if_datamov > 0);
	  
                                                     
                                             
    
	if (--ifp->if_datamov == 0 && ifp->if_drainers > 0) {
		wakeup(&(ifp->if_datamov));
	}
	ifnet_decr_iorefcnt_locked(ifp);
	lck_mtx_unlock(&ifp->if_ref_lock);
}

void
ifnet_datamov_suspend(struct ifnet *ifp)
{
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	VERIFY(ifp->if_refflags & (IFRF_ATTACHED | IFRF_DETACHING));
	ifp->if_refio++;
	if (ifp->if_suspend++ == 0) {
		VERIFY(ifp->if_refflags & IFRF_READY);
		ifp->if_refflags &= ~IFRF_READY;
	}
	lck_mtx_unlock(&ifp->if_ref_lock);
}

void
ifnet_datamov_drain(struct ifnet *ifp)
{
	lck_mtx_lock(&ifp->if_ref_lock);
	VERIFY(ifp->if_refflags & (IFRF_ATTACHED | IFRF_DETACHING));
	                                             
	VERIFY(ifp->if_suspend > 0);
	VERIFY(!(ifp->if_refflags & IFRF_READY));
	ifp->if_drainers++;
	while (ifp->if_datamov != 0) {
		(void) msleep(&(ifp->if_datamov), &ifp->if_ref_lock,
		    (PZERO - 1), __func__, NULL);
	}
	VERIFY(!(ifp->if_refflags & IFRF_READY));
	VERIFY(ifp->if_drainers > 0);
	ifp->if_drainers--;
	lck_mtx_unlock(&ifp->if_ref_lock);

	                                
	if ((ifp->if_eflags & IFEF_TXSTART) != 0) {
		if_qflush(ifp, 0);
	}
}

void
ifnet_datamov_resume(struct ifnet *ifp)
{
	lck_mtx_lock(&ifp->if_ref_lock);
	                                             
	VERIFY(ifp->if_suspend > 0);
	if (--ifp->if_suspend == 0) {
		VERIFY(!(ifp->if_refflags & IFRF_READY));
		ifp->if_refflags |= IFRF_READY;
	}
	ifnet_decr_iorefcnt_locked(ifp);
	lck_mtx_unlock(&ifp->if_ref_lock);
}

static void
dlil_if_trace(struct dlil_ifnet *dl_if, int refhold)
{
	struct dlil_ifnet_dbg *dl_if_dbg = (struct dlil_ifnet_dbg *)dl_if;
	ctrace_t *tr;
	u_int32_t idx;
	u_int16_t *cnt;

	if (!(dl_if->dl_if_flags & DLIF_DEBUG)) {
		panic("%s: dl_if %p has no debug structure", __func__, dl_if);
		                
	}

	if (refhold) {
		cnt = &dl_if_dbg->dldbg_if_refhold_cnt;
		tr = dl_if_dbg->dldbg_if_refhold;
	} else {
		cnt = &dl_if_dbg->dldbg_if_refrele_cnt;
		tr = dl_if_dbg->dldbg_if_refrele;
	}

	idx = atomic_add_16_ov(cnt, 1) % IF_REF_TRACE_HIST_SIZE;
	ctrace_record(&tr[idx]);
}

errno_t
dlil_if_ref(struct ifnet *ifp)
{
	struct dlil_ifnet *dl_if = (struct dlil_ifnet *)ifp;

	if (dl_if == NULL) {
		return EINVAL;
	}

	lck_mtx_lock_spin(&dl_if->dl_if_lock);
	++dl_if->dl_if_refcnt;
	if (dl_if->dl_if_refcnt == 0) {
		panic("%s: wraparound refcnt for ifp=%p", __func__, ifp);
		                
	}
	if (dl_if->dl_if_trace != NULL) {
		(*dl_if->dl_if_trace)(dl_if, TRUE);
	}
	lck_mtx_unlock(&dl_if->dl_if_lock);

	return 0;
}

errno_t
dlil_if_free(struct ifnet *ifp)
{
	struct dlil_ifnet *dl_if = (struct dlil_ifnet *)ifp;
	bool need_release = FALSE;

	if (dl_if == NULL) {
		return EINVAL;
	}

	lck_mtx_lock_spin(&dl_if->dl_if_lock);
	switch (dl_if->dl_if_refcnt) {
	case 0:
		panic("%s: negative refcnt for ifp=%p", __func__, ifp);
		                
		break;
	case 1:
		if ((ifp->if_refflags & IFRF_EMBRYONIC) != 0) {
			need_release = TRUE;
		}
		break;
	default:
		break;
	}
	--dl_if->dl_if_refcnt;
	if (dl_if->dl_if_trace != NULL) {
		(*dl_if->dl_if_trace)(dl_if, FALSE);
	}
	lck_mtx_unlock(&dl_if->dl_if_lock);
	if (need_release) {
		dlil_if_release(ifp);
	}
	return 0;
}

static errno_t
dlil_attach_protocol_internal(struct if_proto *proto,
    const struct ifnet_demux_desc *demux_list, u_int32_t demux_count,
    uint32_t * proto_count)
{
	struct kev_dl_proto_data ev_pr_data;
	struct ifnet *ifp = proto->ifp;
	int retval = 0;
	u_int32_t hash_value = proto_hash_value(proto->protocol_family);
	struct if_proto *prev_proto;
	struct if_proto *_proto;

	                                              
	ifnet_lock_exclusive(ifp);
	_proto = find_attached_proto(ifp, proto->protocol_family);
	if (_proto != NULL) {
		ifnet_lock_done(ifp);
		if_proto_free(_proto);
		return EEXIST;
	}

	  
                                                             
                                   
    
	retval = ifp->if_add_proto(ifp, proto->protocol_family, demux_list,
	    demux_count);
	if (retval) {
		ifnet_lock_done(ifp);
		return retval;
	}

	  
                                   
    
	prev_proto = SLIST_FIRST(&ifp->if_proto_hash[hash_value]);
	while (prev_proto != NULL && SLIST_NEXT(prev_proto, next_hash) != NULL) {
		prev_proto = SLIST_NEXT(prev_proto, next_hash);
	}
	if (prev_proto) {
		SLIST_INSERT_AFTER(prev_proto, proto, next_hash);
	} else {
		SLIST_INSERT_HEAD(&ifp->if_proto_hash[hash_value],
		    proto, next_hash);
	}

	                                    
	if_proto_ref(proto);

	  
                                                                    
                       
    
	ev_pr_data.proto_family = proto->protocol_family;
	ev_pr_data.proto_remaining_count = dlil_ifp_protolist(ifp, NULL, 0);

	ifnet_lock_done(ifp);

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_PROTO_ATTACHED,
	    (struct net_event_data *)&ev_pr_data,
	    sizeof(struct kev_dl_proto_data));
	if (proto_count != NULL) {
		*proto_count = ev_pr_data.proto_remaining_count;
	}
	return retval;
}

errno_t
ifnet_attach_protocol(ifnet_t ifp, protocol_family_t protocol,
    const struct ifnet_attach_proto_param *proto_details)
{
	int retval = 0;
	struct if_proto  *ifproto = NULL;
	uint32_t proto_count = 0;

	ifnet_head_lock_shared();
	if (ifp == NULL || protocol == 0 || proto_details == NULL) {
		retval = EINVAL;
		goto end;
	}
	                                                    
	if (!ifnet_lookup(ifp)) {
		retval = ENXIO;
		goto end;
	}

	ifproto = zalloc_flags(dlif_proto_zone, Z_WAITOK | Z_ZERO);
	if (ifproto == NULL) {
		retval = ENOMEM;
		goto end;
	}

	                                     
	ifproto->ifp = ifp;
	ifproto->protocol_family = protocol;
	ifproto->proto_kpi = kProtoKPI_v1;
	ifproto->kpi.v1.input = proto_details->input;
	ifproto->kpi.v1.pre_output = proto_details->pre_output;
	ifproto->kpi.v1.event = proto_details->event;
	ifproto->kpi.v1.ioctl = proto_details->ioctl;
	ifproto->kpi.v1.detached = proto_details->detached;
	ifproto->kpi.v1.resolve_multi = proto_details->resolve;
	ifproto->kpi.v1.send_arp = proto_details->send_arp;

	retval = dlil_attach_protocol_internal(ifproto,
	    proto_details->demux_list, proto_details->demux_count,
	    &proto_count);

end:
	if (retval != 0 && retval != EEXIST) {
		DLIL_PRINTF("%s: failed to attach v1 protocol %d (err=%d)\n",
		    ifp != NULL ? if_name(ifp) : "N/A", protocol, retval);
	} else {
		if (dlil_verbose) {
			DLIL_PRINTF("%s: attached v1 protocol %d (count = %d)\n",
			    ifp != NULL ? if_name(ifp) : "N/A",
			    protocol, proto_count);
		}
	}
	ifnet_head_done();
	if (retval == 0) {
		  
                                                         
                                                                 
                                                            
     
		(void) ifnet_set_flags(ifp, IFF_UP, IFF_UP);
		(void) ifnet_ioctl(ifp, 0, SIOCSIFFLAGS, NULL);
		dlil_post_sifflags_msg(ifp);
	} else if (ifproto != NULL) {
		zfree(dlif_proto_zone, ifproto);
	}
	return retval;
}

errno_t
ifnet_attach_protocol_v2(ifnet_t ifp, protocol_family_t protocol,
    const struct ifnet_attach_proto_param_v2 *proto_details)
{
	int retval = 0;
	struct if_proto  *ifproto = NULL;
	uint32_t proto_count = 0;

	ifnet_head_lock_shared();
	if (ifp == NULL || protocol == 0 || proto_details == NULL) {
		retval = EINVAL;
		goto end;
	}
	                                                    
	if (!ifnet_lookup(ifp)) {
		retval = ENXIO;
		goto end;
	}

	ifproto = zalloc(dlif_proto_zone);
	if (ifproto == NULL) {
		retval = ENOMEM;
		goto end;
	}
	bzero(ifproto, sizeof(*ifproto));

	                                     
	ifproto->ifp = ifp;
	ifproto->protocol_family = protocol;
	ifproto->proto_kpi = kProtoKPI_v2;
	ifproto->kpi.v2.input = proto_details->input;
	ifproto->kpi.v2.pre_output = proto_details->pre_output;
	ifproto->kpi.v2.event = proto_details->event;
	ifproto->kpi.v2.ioctl = proto_details->ioctl;
	ifproto->kpi.v2.detached = proto_details->detached;
	ifproto->kpi.v2.resolve_multi = proto_details->resolve;
	ifproto->kpi.v2.send_arp = proto_details->send_arp;

	retval = dlil_attach_protocol_internal(ifproto,
	    proto_details->demux_list, proto_details->demux_count,
	    &proto_count);

end:
	if (retval != 0 && retval != EEXIST) {
		DLIL_PRINTF("%s: failed to attach v2 protocol %d (err=%d)\n",
		    ifp != NULL ? if_name(ifp) : "N/A", protocol, retval);
	} else {
		if (dlil_verbose) {
			DLIL_PRINTF("%s: attached v2 protocol %d (count = %d)\n",
			    ifp != NULL ? if_name(ifp) : "N/A",
			    protocol, proto_count);
		}
	}
	ifnet_head_done();
	if (retval == 0) {
		  
                                                         
                                                                 
                                                            
     
		(void) ifnet_set_flags(ifp, IFF_UP, IFF_UP);
		(void) ifnet_ioctl(ifp, 0, SIOCSIFFLAGS, NULL);
		dlil_post_sifflags_msg(ifp);
	} else if (ifproto != NULL) {
		zfree(dlif_proto_zone, ifproto);
	}
	return retval;
}

errno_t
ifnet_detach_protocol(ifnet_t ifp, protocol_family_t proto_family)
{
	struct if_proto *proto = NULL;
	int     retval = 0;

	if (ifp == NULL || proto_family == 0) {
		retval = EINVAL;
		goto end;
	}

	ifnet_lock_exclusive(ifp);
	                                              
	proto = find_attached_proto(ifp, proto_family);
	if (proto == NULL) {
		retval = ENXIO;
		ifnet_lock_done(ifp);
		goto end;
	}

	                                  
	if (ifp->if_del_proto) {
		ifp->if_del_proto(ifp, proto->protocol_family);
	}

	SLIST_REMOVE(&ifp->if_proto_hash[proto_hash_value(proto_family)],
	    proto, if_proto, next_hash);

	if (proto->proto_kpi == kProtoKPI_v1) {
		proto->kpi.v1.input = ifproto_media_input_v1;
		proto->kpi.v1.pre_output = ifproto_media_preout;
		proto->kpi.v1.event = ifproto_media_event;
		proto->kpi.v1.ioctl = ifproto_media_ioctl;
		proto->kpi.v1.resolve_multi = ifproto_media_resolve_multi;
		proto->kpi.v1.send_arp = ifproto_media_send_arp;
	} else {
		proto->kpi.v2.input = ifproto_media_input_v2;
		proto->kpi.v2.pre_output = ifproto_media_preout;
		proto->kpi.v2.event = ifproto_media_event;
		proto->kpi.v2.ioctl = ifproto_media_ioctl;
		proto->kpi.v2.resolve_multi = ifproto_media_resolve_multi;
		proto->kpi.v2.send_arp = ifproto_media_send_arp;
	}
	proto->detached = 1;
	ifnet_lock_done(ifp);

	if (dlil_verbose) {
		DLIL_PRINTF("%s: detached %s protocol %d\n", if_name(ifp),
		    (proto->proto_kpi == kProtoKPI_v1) ?
		    "v1" : "v2", proto_family);
	}

	                                                      
	if_proto_free(proto);

	  
                                                        
                                                         
                          
    
	if_proto_free(proto);

end:
	return retval;
}


static errno_t
ifproto_media_input_v1(struct ifnet *ifp, protocol_family_t protocol,
    struct mbuf *packet, char *header)
{
#pragma unused(ifp, protocol, packet, header)
	return ENXIO;
}

static errno_t
ifproto_media_input_v2(struct ifnet *ifp, protocol_family_t protocol,
    struct mbuf *packet)
{
#pragma unused(ifp, protocol, packet)
	return ENXIO;
}

static errno_t
ifproto_media_preout(struct ifnet *ifp, protocol_family_t protocol,
    mbuf_t *packet, const struct sockaddr *dest, void *route, char *frame_type,
    char *link_layer_dest)
{
#pragma unused(ifp, protocol, packet, dest, route, frame_type, link_layer_dest)
	return ENXIO;
}

static void
ifproto_media_event(struct ifnet *ifp, protocol_family_t protocol,
    const struct kev_msg *event)
{
#pragma unused(ifp, protocol, event)
}

static errno_t
ifproto_media_ioctl(struct ifnet *ifp, protocol_family_t protocol,
    unsigned long command, void *argument)
{
#pragma unused(ifp, protocol, command, argument)
	return ENXIO;
}

static errno_t
ifproto_media_resolve_multi(ifnet_t ifp, const struct sockaddr *proto_addr,
    struct sockaddr_dl *out_ll, size_t ll_len)
{
#pragma unused(ifp, proto_addr, out_ll, ll_len)
	return ENXIO;
}

static errno_t
ifproto_media_send_arp(struct ifnet *ifp, u_short arpop,
    const struct sockaddr_dl *sender_hw, const struct sockaddr *sender_proto,
    const struct sockaddr_dl *target_hw, const struct sockaddr *target_proto)
{
#pragma unused(ifp, arpop, sender_hw, sender_proto, target_hw, target_proto)
	return ENXIO;
}

extern int if_next_index(void);
extern int tcp_ecn_outbound;

errno_t
ifnet_attach(ifnet_t ifp, const struct sockaddr_dl *ll_addr)
{
	struct ifnet *tmp_if;
	struct ifaddr *ifa;
	struct if_data_internal if_data_saved;
	struct dlil_ifnet *dl_if = (struct dlil_ifnet *)ifp;
	struct dlil_threading_info *dl_inp;
	thread_continue_t thfunc = NULL;
	u_int32_t sflags = 0;
	int err;

	if (ifp == NULL) {
		return EINVAL;
	}

	  
                                                             
                                                           
                                                           
                                                        
    
	dlil_if_lock();
	ifnet_head_lock_exclusive();
	                                          
	TAILQ_FOREACH(tmp_if, &ifnet_head, if_link) {
		if (tmp_if == ifp) {
			ifnet_head_done();
			dlil_if_unlock();
			return EEXIST;
		}
	}

	lck_mtx_lock_spin(&ifp->if_ref_lock);
	if (!(ifp->if_refflags & IFRF_EMBRYONIC)) {
		panic_plain("%s: flags mismatch (embryonic not set) ifp=%p",
		    __func__, ifp);
		                
	}
	lck_mtx_unlock(&ifp->if_ref_lock);

	ifnet_lock_exclusive(ifp);

	                  
	VERIFY(ifp->if_detaching_link.tqe_next == NULL);
	VERIFY(ifp->if_detaching_link.tqe_prev == NULL);
	VERIFY(ifp->if_threads_pending == 0);

	if (ll_addr != NULL) {
		if (ifp->if_addrlen == 0) {
			ifp->if_addrlen = ll_addr->sdl_alen;
		} else if (ll_addr->sdl_alen != ifp->if_addrlen) {
			ifnet_lock_done(ifp);
			ifnet_head_done();
			dlil_if_unlock();
			return EINVAL;
		}
	}

	  
                                                        
                                                      
    
	if (ifp->if_add_proto == NULL || ifp->if_del_proto == NULL) {
		DLIL_PRINTF("%s: Attempt to attach interface without "
		    "family module - %d\n", __func__, ifp->if_family);
		ifnet_lock_done(ifp);
		ifnet_head_done();
		dlil_if_unlock();
		return ENODEV;
	}

	                                  
	VERIFY(ifp->if_proto_hash == NULL);
	ifp->if_proto_hash = zalloc_flags(dlif_phash_zone, Z_WAITOK | Z_ZERO);
	if (ifp->if_proto_hash == NULL) {
		ifnet_lock_done(ifp);
		ifnet_head_done();
		dlil_if_unlock();
		return ENOBUFS;
	}

	lck_mtx_lock_spin(&ifp->if_flt_lock);
	VERIFY(TAILQ_EMPTY(&ifp->if_flt_head));
	TAILQ_INIT(&ifp->if_flt_head);
	VERIFY(ifp->if_flt_busy == 0);
	VERIFY(ifp->if_flt_waiters == 0);
	lck_mtx_unlock(&ifp->if_flt_lock);

	if (!(dl_if->dl_if_flags & DLIF_REUSE)) {
		VERIFY(LIST_EMPTY(&ifp->if_multiaddrs));
		LIST_INIT(&ifp->if_multiaddrs);
	}

	VERIFY(ifp->if_allhostsinm == NULL);
	VERIFY(TAILQ_EMPTY(&ifp->if_addrhead));
	TAILQ_INIT(&ifp->if_addrhead);

	if (ifp->if_index == 0) {
		int idx = if_next_index();

		if (idx == -1) {
			ifp->if_index = 0;
			ifnet_lock_done(ifp);
			ifnet_head_done();
			dlil_if_unlock();
			return ENOBUFS;
		}
		ifp->if_index = (uint16_t)idx;

		                                                               
		if (ll_addr != NULL && ifp->if_type == IFT_ETHER &&
		    ll_addr->sdl_alen == ETHER_ADDR_LEN) {
			bcopy(CONST_LLADDR(ll_addr),
			    dl_if->dl_if_permanent_ether,
			    ETHER_ADDR_LEN);
			dl_if->dl_if_permanent_ether_is_set = 1;
		}
	}
	                                                      
	VERIFY(ifindex2ifnet[ifp->if_index] == NULL);

	                                                        
	ifa = dlil_alloc_lladdr(ifp, ll_addr);
	if (ifa == NULL) {
		ifnet_lock_done(ifp);
		ifnet_head_done();
		dlil_if_unlock();
		return ENOBUFS;
	}

	VERIFY(ifnet_addrs[ifp->if_index - 1] == NULL);
	ifnet_addrs[ifp->if_index - 1] = ifa;

	                                             
	IFA_LOCK(ifa);
	                                        
	IFA_ADDREF_LOCKED(ifa);
	                                                         
	if_attach_link_ifa(ifp, ifa);
	IFA_UNLOCK(ifa);

	TAILQ_INSERT_TAIL(&ifnet_head, ifp, if_link);
	ifindex2ifnet[ifp->if_index] = ifp;

	                                                   
	ifnet_reference(ifp);

	                                                              
	if_data_saved = ifp->if_data;
	bzero(&ifp->if_data, sizeof(ifp->if_data));
	ifp->if_data.ifi_type = if_data_saved.ifi_type;
	ifp->if_data.ifi_typelen = if_data_saved.ifi_typelen;
	ifp->if_data.ifi_physical = if_data_saved.ifi_physical;
	ifp->if_data.ifi_addrlen = if_data_saved.ifi_addrlen;
	ifp->if_data.ifi_hdrlen = if_data_saved.ifi_hdrlen;
	ifp->if_data.ifi_mtu = if_data_saved.ifi_mtu;
	ifp->if_data.ifi_baudrate = if_data_saved.ifi_baudrate;
	ifp->if_data.ifi_hwassist = if_data_saved.ifi_hwassist;
	ifp->if_data.ifi_tso_v4_mtu = if_data_saved.ifi_tso_v4_mtu;
	ifp->if_data.ifi_tso_v6_mtu = if_data_saved.ifi_tso_v6_mtu;
	ifnet_touch_lastchange(ifp);

	VERIFY(ifp->if_output_sched_model == IFNET_SCHED_MODEL_NORMAL ||
	    ifp->if_output_sched_model == IFNET_SCHED_MODEL_DRIVER_MANAGED ||
	    ifp->if_output_sched_model == IFNET_SCHED_MODEL_FQ_CODEL);

	                                                  
	sflags = PKTSCHEDF_QALG_SFB;
	if (if_flowadv) {
		sflags |= PKTSCHEDF_QALG_FLOWCTL;
	}

	if (if_delaybased_queue) {
		sflags |= PKTSCHEDF_QALG_DELAYBASED;
	}

	if (ifp->if_output_sched_model ==
	    IFNET_SCHED_MODEL_DRIVER_MANAGED) {
		sflags |= PKTSCHEDF_QALG_DRIVER_MANAGED;
	}

	                                  
	err = ifclassq_setup(ifp, sflags, (dl_if->dl_if_flags & DLIF_REUSE));
	if (err != 0) {
		panic_plain("%s: ifp=%p couldn't initialize transmit queue; "
		    "err=%d", __func__, ifp, err);
		                
	}

	                                               
	dl_inp = &dl_if->dl_if_inpstorage;
	bzero(&dl_inp->dlth_stats, sizeof(dl_inp->dlth_stats));
	VERIFY(dl_inp->dlth_flags == 0);
	VERIFY(dl_inp->dlth_wtot == 0);
	VERIFY(dl_inp->dlth_ifp == NULL);
	VERIFY(qhead(&dl_inp->dlth_pkts) == NULL && qempty(&dl_inp->dlth_pkts));
	VERIFY(qlimit(&dl_inp->dlth_pkts) == 0);
	VERIFY(!dl_inp->dlth_affinity);
	VERIFY(ifp->if_inp == NULL);
	VERIFY(dl_inp->dlth_thread == THREAD_NULL);
	VERIFY(dl_inp->dlth_strategy == NULL);
	VERIFY(dl_inp->dlth_driver_thread == THREAD_NULL);
	VERIFY(dl_inp->dlth_poller_thread == THREAD_NULL);
	VERIFY(dl_inp->dlth_affinity_tag == 0);

#if IFNET_INPUT_SANITY_CHK
	VERIFY(dl_inp->dlth_pkts_cnt == 0);
#endif                             

	VERIFY(ifp->if_poll_thread == THREAD_NULL);
	dlil_reset_rxpoll_params(ifp);
	  
                                                                       
    
	if (ifp->if_family != IFNET_FAMILY_LOOPBACK) {
		ifp->if_inp = dl_inp;
		ifnet_incr_pending_thread_count(ifp);
		err = dlil_create_input_thread(ifp, ifp->if_inp, &thfunc);
		if (err == ENODEV) {
			VERIFY(thfunc == NULL);
			ifnet_decr_pending_thread_count(ifp);
		} else if (err != 0) {
			panic_plain("%s: ifp=%p couldn't get an input thread; "
			    "err=%d", __func__, ifp, err);
			                
		}
	}
	  
                                                                      
                                                                        
                                                      
    
	if (ifp->if_eflags & IFEF_TXSTART) {
		thread_precedence_policy_data_t info;
		__unused kern_return_t kret;

		ifp->if_flowhash = ifnet_calc_flowhash(ifp);
		VERIFY(ifp->if_flowhash != 0);
		VERIFY(ifp->if_start_thread == THREAD_NULL);

		ifnet_set_start_cycle(ifp, NULL);
		ifp->if_start_active = 0;
		ifp->if_start_req = 0;
		ifp->if_start_flags = 0;
		VERIFY(ifp->if_start != NULL);
		ifnet_incr_pending_thread_count(ifp);
		if ((err = kernel_thread_start(ifnet_start_thread_func,
		    ifp, &ifp->if_start_thread)) != KERN_SUCCESS) {
			panic_plain("%s: "
			    "ifp=%p couldn't get a start thread; "
			    "err=%d", __func__, ifp, err);
			                
		}
		bzero(&info, sizeof(info));
		info.importance = 1;
		kret = thread_policy_set(ifp->if_start_thread,
		    THREAD_PRECEDENCE_POLICY, (thread_policy_t)&info,
		    THREAD_PRECEDENCE_POLICY_COUNT);
		ASSERT(kret == KERN_SUCCESS);
	} else {
		ifp->if_flowhash = 0;
	}

	                              
	ifnet_set_poll_cycle(ifp, NULL);
	ifp->if_poll_update = 0;
	ifp->if_poll_flags = 0;
	ifp->if_poll_req = 0;
	VERIFY(ifp->if_poll_thread == THREAD_NULL);

	  
                                                                 
                                                                 
                                                            
                                                              
                     
    
	if (thfunc == dlil_rxpoll_input_thread_func) {
		thread_precedence_policy_data_t info;
		__unused kern_return_t kret;
		VERIFY(ifp->if_input_poll != NULL);
		VERIFY(ifp->if_input_ctl != NULL);
		ifnet_incr_pending_thread_count(ifp);
		if ((err = kernel_thread_start(ifnet_poll_thread_func, ifp,
		    &ifp->if_poll_thread)) != KERN_SUCCESS) {
			panic_plain("%s: ifp=%p couldn't get a poll thread; "
			    "err=%d", __func__, ifp, err);
			                
		}
		bzero(&info, sizeof(info));
		info.importance = 1;
		kret = thread_policy_set(ifp->if_poll_thread,
		    THREAD_PRECEDENCE_POLICY, (thread_policy_t)&info,
		    THREAD_PRECEDENCE_POLICY_COUNT);
		ASSERT(kret == KERN_SUCCESS);
	}

	VERIFY(ifp->if_desc.ifd_maxlen == IF_DESCSIZE);
	VERIFY(ifp->if_desc.ifd_len == 0);
	VERIFY(ifp->if_desc.ifd_desc != NULL);

	                                 
	ctrace_record(&((struct dlil_ifnet *)ifp)->dl_if_attach);

	ifp->if_updatemcasts = 0;
	if (!LIST_EMPTY(&ifp->if_multiaddrs)) {
		struct ifmultiaddr *ifma;
		LIST_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
			IFMA_LOCK(ifma);
			if (ifma->ifma_addr->sa_family == AF_LINK ||
			    ifma->ifma_addr->sa_family == AF_UNSPEC) {
				ifp->if_updatemcasts++;
			}
			IFMA_UNLOCK(ifma);
		}

		DLIL_PRINTF("%s: attached with %d suspended link-layer multicast "
		    "membership(s)\n", if_name(ifp),
		    ifp->if_updatemcasts);
	}

	                              
	bzero(&ifp->if_log, sizeof(ifp->if_log));

	                                                   
	ifp->if_fg_sendts = 0;
	ifp->if_rt_sendts = 0;

	VERIFY(ifp->if_delegated.ifp == NULL);
	VERIFY(ifp->if_delegated.type == 0);
	VERIFY(ifp->if_delegated.family == 0);
	VERIFY(ifp->if_delegated.subfamily == 0);
	VERIFY(ifp->if_delegated.expensive == 0);
	VERIFY(ifp->if_delegated.constrained == 0);

	VERIFY(ifp->if_agentids == NULL);
	VERIFY(ifp->if_agentcount == 0);

	                           
	bzero(&ifp->if_interface_state, sizeof(ifp->if_interface_state));
	ifp->if_interface_state.valid_bitmask |=
	    IF_INTERFACE_STATE_INTERFACE_AVAILABILITY_VALID;
	ifp->if_interface_state.interface_availability =
	    IF_INTERFACE_STATE_INTERFACE_AVAILABLE;

	                                                                    
	if (ifp == lo_ifp) {
		ifp->if_interface_state.lqm_state = IFNET_LQM_THRESH_GOOD;
		ifp->if_interface_state.valid_bitmask |=
		    IF_INTERFACE_STATE_LQM_STATE_VALID;
	} else {
		ifp->if_interface_state.lqm_state = IFNET_LQM_THRESH_UNKNOWN;
	}

	  
                                                            
                               
    
	if (tcp_ecn_outbound == 2 && !IFNET_IS_CELLULAR(ifp)) {
		if_set_eflags(ifp, IFEF_ECN_ENABLE);
		if_clear_eflags(ifp, IFEF_ECN_DISABLE);
	}

	  
                                                    
    
	if (IFNET_IS_WIFI_INFRA(ifp) && net_qos_policy_wifi_enabled != 0) {
		errno_t error;

		error = if_set_qosmarking_mode(ifp,
		    IFRTYPE_QOSMARKING_FASTLANE);
		if (error != 0) {
			DLIL_PRINTF("%s if_set_qosmarking_mode(%s) error %d\n",
			    __func__, ifp->if_xname, error);
		} else {
			if_set_eflags(ifp, IFEF_QOSMARKING_ENABLED);
#if (DEVELOPMENT || DEBUG)
			DLIL_PRINTF("%s fastlane enabled on %s\n",
			    __func__, ifp->if_xname);
#endif                             
		}
	}

	ifnet_lock_done(ifp);
	ifnet_head_done();


	lck_mtx_lock(&ifp->if_cached_route_lock);
	                                    
	ifp->if_fwd_cacheok = 1;
	                                         
	ROUTE_RELEASE(&ifp->if_fwd_route);
	bzero(&ifp->if_fwd_route, sizeof(ifp->if_fwd_route));
	ROUTE_RELEASE(&ifp->if_src_route);
	bzero(&ifp->if_src_route, sizeof(ifp->if_src_route));
	ROUTE_RELEASE(&ifp->if_src_route6);
	bzero(&ifp->if_src_route6, sizeof(ifp->if_src_route6));
	lck_mtx_unlock(&ifp->if_cached_route_lock);

	ifnet_llreach_ifattach(ifp, (dl_if->dl_if_flags & DLIF_REUSE));

	  
                                                                 
                                                              
                                                                   
                                                                
                                                            
                                                                
                              
    
#if INET
	if (IGMP_IFINFO(ifp) == NULL) {
		IGMP_IFINFO(ifp) = igmp_domifattach(ifp, Z_WAITOK);
		VERIFY(IGMP_IFINFO(ifp) != NULL);
	} else {
		VERIFY(IGMP_IFINFO(ifp)->igi_ifp == ifp);
		igmp_domifreattach(IGMP_IFINFO(ifp));
	}
#endif           
	if (MLD_IFINFO(ifp) == NULL) {
		MLD_IFINFO(ifp) = mld_domifattach(ifp, Z_WAITOK);
		VERIFY(MLD_IFINFO(ifp) != NULL);
	} else {
		VERIFY(MLD_IFINFO(ifp)->mli_ifp == ifp);
		mld_domifreattach(MLD_IFINFO(ifp));
	}

	VERIFY(ifp->if_data_threshold == 0);
	VERIFY(ifp->if_dt_tcall != NULL);

	  
                                                      
                                                     
                                  
    
	lck_mtx_lock(&ifp->if_ref_lock);
	while (ifp->if_threads_pending != 0) {
		DLIL_PRINTF("%s: Waiting for all kernel threads created for "
		    "interface %s to get scheduled at least once.\n",
		    __func__, ifp->if_xname);
		(void) msleep(&ifp->if_threads_pending, &ifp->if_ref_lock, (PZERO - 1),
		    __func__, NULL);
		LCK_MTX_ASSERT(&ifp->if_ref_lock, LCK_ASSERT_OWNED);
	}
	lck_mtx_unlock(&ifp->if_ref_lock);
	DLIL_PRINTF("%s: All kernel threads created for interface %s have been scheduled "
	    "at least once. Proceeding.\n", __func__, ifp->if_xname);

	                                        
	lck_mtx_lock(rnh_lock);
	ifnet_lock_exclusive(ifp);
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	ifp->if_refflags = (IFRF_ATTACHED | IFRF_READY);                       
	lck_mtx_unlock(&ifp->if_ref_lock);
	if (net_rtref) {
		                                                  
		(void) ifnet_set_idle_flags_locked(ifp, IFRF_IDLE_NOTIFY,
		    IFRF_IDLE_NOTIFY);
	} else {
		                                                             
		(void) ifnet_set_idle_flags_locked(ifp, ifp->if_idle_new_flags,
		    ifp->if_idle_new_flags_mask);
	}
	ifnet_lock_done(ifp);
	lck_mtx_unlock(rnh_lock);
	dlil_if_unlock();

#if PF
	  
                                                       
    
	pf_ifnet_hook(ifp, 1);
#endif         

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_IF_ATTACHED, NULL, 0);

	if (dlil_verbose) {
		DLIL_PRINTF("%s: attached%s\n", if_name(ifp),
		    (dl_if->dl_if_flags & DLIF_REUSE) ? " (recycled)" : "");
	}

	return 0;
}

  
                                                                       
                                                                      
                                                                          
                                                                       
                                                                        
                                 
  
                                         
   
static struct ifaddr *
dlil_alloc_lladdr(struct ifnet *ifp, const struct sockaddr_dl *ll_addr)
{
	struct ifaddr *ifa, *oifa;
	struct sockaddr_dl *asdl, *msdl;
	char workbuf[IFNAMSIZ * 2];
	int namelen, masklen, socksize;
	struct dlil_ifnet *dl_if = (struct dlil_ifnet *)ifp;

	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_EXCLUSIVE);
	VERIFY(ll_addr == NULL || ll_addr->sdl_alen == ifp->if_addrlen);

	namelen = scnprintf(workbuf, sizeof(workbuf), "%s",
	    if_name(ifp));
	masklen = offsetof(struct sockaddr_dl, sdl_data[0])
	    + ((namelen > 0) ? namelen : 0);
	socksize = masklen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof (u_int32_t) - 1)))
	if ((u_int32_t)socksize < sizeof(struct sockaddr_dl)) {
		socksize = sizeof(struct sockaddr_dl);
	}
	socksize = ROUNDUP(socksize);
#undef ROUNDUP

	ifa = ifp->if_lladdr;
	if (socksize > DLIL_SDLMAXLEN ||
	    (ifa != NULL && ifa != &dl_if->dl_if_lladdr.ifa)) {
		  
                                                          
                                                         
                                                         
                                                            
                                                          
     
		if (ifa == NULL || ifa == &dl_if->dl_if_lladdr.ifa) {
			int ifasize = sizeof(*ifa) + 2 * SOCK_MAXADDRLEN;
			ifa = _MALLOC(ifasize, M_IFADDR, M_WAITOK | M_ZERO);
			if (ifa == NULL) {
				return NULL;
			}
			ifa_lock_init(ifa);
			                                               
			ifa->ifa_debug = IFD_LINK;
		}
		IFA_LOCK(ifa);
		                                            
		asdl = (struct sockaddr_dl *)(ifa + 1);
		bzero(asdl, SOCK_MAXADDRLEN);
		msdl = (struct sockaddr_dl *)(void *)
		    ((char *)asdl + SOCK_MAXADDRLEN);
		bzero(msdl, SOCK_MAXADDRLEN);
	} else {
		VERIFY(ifa == NULL || ifa == &dl_if->dl_if_lladdr.ifa);
		  
                                                          
                                                         
     
		if (ifa == NULL) {
			ifa = &dl_if->dl_if_lladdr.ifa;
			ifa_lock_init(ifa);
			                                               
			ifa->ifa_debug = IFD_LINK;
		}
		IFA_LOCK(ifa);
		                                            
		asdl = (struct sockaddr_dl *)(void *)&dl_if->dl_if_lladdr.asdl;
		bzero(asdl, sizeof(dl_if->dl_if_lladdr.asdl));
		msdl = (struct sockaddr_dl *)(void *)&dl_if->dl_if_lladdr.msdl;
		bzero(msdl, sizeof(dl_if->dl_if_lladdr.msdl));
	}

	                                                     
	IFA_ADDREF_LOCKED(ifa);
	oifa = ifp->if_lladdr;
	ifp->if_lladdr = ifa;

	VERIFY(ifa->ifa_debug == IFD_LINK);
	ifa->ifa_ifp = ifp;
	ifa->ifa_rtrequest = link_rtrequest;
	ifa->ifa_addr = (struct sockaddr *)asdl;
	asdl->sdl_len = (u_char)socksize;
	asdl->sdl_family = AF_LINK;
	if (namelen > 0) {
		bcopy(workbuf, asdl->sdl_data, min(namelen,
		    sizeof(asdl->sdl_data)));
		asdl->sdl_nlen = (u_char)namelen;
	} else {
		asdl->sdl_nlen = 0;
	}
	asdl->sdl_index = ifp->if_index;
	asdl->sdl_type = ifp->if_type;
	if (ll_addr != NULL) {
		asdl->sdl_alen = ll_addr->sdl_alen;
		bcopy(CONST_LLADDR(ll_addr), LLADDR(asdl), asdl->sdl_alen);
	} else {
		asdl->sdl_alen = 0;
	}
	ifa->ifa_netmask = (struct sockaddr *)msdl;
	msdl->sdl_len = (u_char)masklen;
	while (namelen > 0) {
		msdl->sdl_data[--namelen] = 0xff;
	}
	IFA_UNLOCK(ifa);

	if (oifa != NULL) {
		IFA_REMREF(oifa);
	}

	return ifa;
}

static void
if_purgeaddrs(struct ifnet *ifp)
{
#if INET
	in_purgeaddrs(ifp);
#endif           
	in6_purgeaddrs(ifp);
}

errno_t
ifnet_detach(ifnet_t ifp)
{
	struct ifnet *delegated_ifp;
	struct nd_ifinfo *ndi = NULL;

	if (ifp == NULL) {
		return EINVAL;
	}

	ndi = ND_IFINFO(ifp);
	if (NULL != ndi) {
		ndi->cga_initialized = FALSE;
	}

	lck_mtx_lock(rnh_lock);
	ifnet_head_lock_exclusive();
	ifnet_lock_exclusive(ifp);

	if (ifp->if_output_netem != NULL) {
		netem_destroy(ifp->if_output_netem);
		ifp->if_output_netem = NULL;
	}

	  
                                                           
                                                             
                                                       
                                                  
    
	(void) ifnet_set_idle_flags_locked(ifp, 0, ~0);

	lck_mtx_lock_spin(&ifp->if_ref_lock);
	if (!(ifp->if_refflags & IFRF_ATTACHED)) {
		lck_mtx_unlock(&ifp->if_ref_lock);
		ifnet_lock_done(ifp);
		ifnet_head_done();
		lck_mtx_unlock(rnh_lock);
		return EINVAL;
	} else if (ifp->if_refflags & IFRF_DETACHING) {
		                                         
		lck_mtx_unlock(&ifp->if_ref_lock);
		ifnet_lock_done(ifp);
		ifnet_head_done();
		lck_mtx_unlock(rnh_lock);
		return ENXIO;
	}
	VERIFY(!(ifp->if_refflags & IFRF_EMBRYONIC));
	                                               
	ifp->if_refflags &= ~IFRF_ATTACHED;
	ifp->if_refflags |= IFRF_DETACHING;
	lck_mtx_unlock(&ifp->if_ref_lock);

	if (dlil_verbose) {
		DLIL_PRINTF("%s: detaching\n", if_name(ifp));
	}

	                                                       
	if (ifp->if_eflags & IFEF_TXSTART) {
		ifnet_flowadv(ifp->if_flowhash);
	}

	                                    
	                       
	if_clear_eflags(ifp, IFEF_ECN_ENABLE | IFEF_ECN_DISABLE | IFEF_CLAT46);

	  
                                                       
                                                         
             
    
	if (ifp->if_tcp_kao_cnt > 0) {
		os_log(OS_LOG_DEFAULT, "%s %s tcp_kao_cnt %u not zero",
		    __func__, if_name(ifp), ifp->if_tcp_kao_cnt);
	}
	ifp->if_tcp_kao_max = 0;

	  
                                                              
                                                        
    
	VERIFY(ifindex2ifnet[ifp->if_index] == ifp);
	TAILQ_REMOVE(&ifnet_head, ifp, if_link);
	ifp->if_link.tqe_next = NULL;
	ifp->if_link.tqe_prev = NULL;
	if (ifp->if_ordered_link.tqe_next != NULL ||
	    ifp->if_ordered_link.tqe_prev != NULL) {
		ifnet_remove_from_ordered_list(ifp);
	}
	ifindex2ifnet[ifp->if_index] = NULL;

	                                  
	if_clear_eflags(ifp, IFEF_IPV4_ROUTER);
	ifp->if_ipv6_router_mode = IPV6_ROUTER_MODE_DISABLED;

	                                 
	ctrace_record(&((struct dlil_ifnet *)ifp)->dl_if_detach);

	                              
	bzero(&ifp->if_log, sizeof(ifp->if_log));

	                                                               
	delegated_ifp = ifp->if_delegated.ifp;
	bzero(&ifp->if_delegated, sizeof(ifp->if_delegated));

	                           
	bzero(&ifp->if_interface_state, sizeof(ifp->if_interface_state));

	ifnet_lock_done(ifp);
	ifnet_head_done();
	lck_mtx_unlock(rnh_lock);


	                                                       
	if (delegated_ifp != NULL) {
		ifnet_release(delegated_ifp);
	}

	                                                       
	if (ifp != lo_ifp) {
		if_lqm_update(ifp, IFNET_LQM_THRESH_OFF, 0);
	}

	                                
	if (ifp->if_tcp_stat != NULL) {
		bzero(ifp->if_tcp_stat, sizeof(*ifp->if_tcp_stat));
	}

	                                
	if (ifp->if_udp_stat != NULL) {
		bzero(ifp->if_udp_stat, sizeof(*ifp->if_udp_stat));
	}

	                            
	if (ifp->if_ipv4_stat != NULL) {
		bzero(ifp->if_ipv4_stat, sizeof(*ifp->if_ipv4_stat));
	}

	                            
	if (ifp->if_ipv6_stat != NULL) {
		bzero(ifp->if_ipv6_stat, sizeof(*ifp->if_ipv6_stat));
	}

	                                                          
	if (ifp->if_link_status != NULL) {
		FREE(ifp->if_link_status, M_TEMP);
		ifp->if_link_status = NULL;
	}

	                     
	if (ifp->if_agentids != NULL) {
		FREE(ifp->if_agentids, M_NETAGENT);
		ifp->if_agentids = NULL;
	}
	ifp->if_agentcount = 0;


	                                  
	bpfdetach(ifp);

	                                
	if_down(ifp);

	                                     
	lck_mtx_lock(&ifp->if_cached_route_lock);
	ifp->if_fwd_cacheok = 0;
	lck_mtx_unlock(&ifp->if_cached_route_lock);

	                                                                   
	ifp->if_data_threshold = 0;
	VERIFY(ifp->if_dt_tcall != NULL);
	(void) thread_call_cancel_wait(ifp->if_dt_tcall);

	  
                                                                 
                                                                
               
    
#if INET
	igmp_domifdetach(ifp);
#endif           
	mld_domifdetach(ifp);

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_IF_DETACHING, NULL, 0);

	                                                                  
	dlil_if_lock();
	ifnet_detaching_enqueue(ifp);
	dlil_if_unlock();

	return 0;
}

static void
ifnet_detaching_enqueue(struct ifnet *ifp)
{
	dlil_if_lock_assert();

	++ifnet_detaching_cnt;
	VERIFY(ifnet_detaching_cnt != 0);
	TAILQ_INSERT_TAIL(&ifnet_detaching_head, ifp, if_detaching_link);
	wakeup((caddr_t)&ifnet_delayed_run);
}

static struct ifnet *
ifnet_detaching_dequeue(void)
{
	struct ifnet *ifp;

	dlil_if_lock_assert();

	ifp = TAILQ_FIRST(&ifnet_detaching_head);
	VERIFY(ifnet_detaching_cnt != 0 || ifp == NULL);
	if (ifp != NULL) {
		VERIFY(ifnet_detaching_cnt != 0);
		--ifnet_detaching_cnt;
		TAILQ_REMOVE(&ifnet_detaching_head, ifp, if_detaching_link);
		ifp->if_detaching_link.tqe_next = NULL;
		ifp->if_detaching_link.tqe_prev = NULL;
	}
	return ifp;
}

__attribute__((noreturn))
static void
ifnet_detacher_thread_cont(void *v, wait_result_t wres)
{
#pragma unused(v, wres)
	struct ifnet *ifp;

	dlil_if_lock();
	if (__improbable(ifnet_detaching_embryonic)) {
		ifnet_detaching_embryonic = FALSE;
		                                                              
		dlil_decr_pending_thread_count();
	}

	for (;;) {
		dlil_if_lock_assert();

		if (ifnet_detaching_cnt == 0) {
			break;
		}

		net_update_uptime();

		VERIFY(TAILQ_FIRST(&ifnet_detaching_head) != NULL);

		                                  
		ifp = ifnet_detaching_dequeue();
		if (ifp != NULL) {
			dlil_if_unlock();
			ifnet_detach_final(ifp);
			dlil_if_lock();
		}
	}

	(void) assert_wait(&ifnet_delayed_run, THREAD_UNINT);
	dlil_if_unlock();
	(void) thread_block(ifnet_detacher_thread_cont);

	VERIFY(0);                                    
	                
	__builtin_unreachable();
}

__dead2
static void
ifnet_detacher_thread_func(void *v, wait_result_t w)
{
#pragma unused(v, w)
	dlil_if_lock();
	(void) assert_wait(&ifnet_delayed_run, THREAD_UNINT);
	ifnet_detaching_embryonic = TRUE;
	                                                
	wakeup((caddr_t)&ifnet_delayed_run);
	dlil_if_unlock();
	(void) thread_block(ifnet_detacher_thread_cont);
	VERIFY(0);
	                
	__builtin_unreachable();
}

static void
ifnet_detach_final(struct ifnet *ifp)
{
	struct ifnet_filter *filter, *filter_next;
	struct ifnet_filter_head fhead;
	struct dlil_threading_info *inp;
	struct ifaddr *ifa;
	ifnet_detached_func if_free;
	int i;

	lck_mtx_lock(&ifp->if_ref_lock);
	if (!(ifp->if_refflags & IFRF_DETACHING)) {
		panic("%s: flags mismatch (detaching not set) ifp=%p",
		    __func__, ifp);
		                
	}

	  
                                                      
                                                       
                                                       
    
	while (ifp->if_refio > 0) {
		DLIL_PRINTF("%s: Waiting for IO references on %s interface "
		    "to be released\n", __func__, if_name(ifp));
		(void) msleep(&(ifp->if_refio), &ifp->if_ref_lock,
		    (PZERO - 1), "ifnet_ioref_wait", NULL);
	}

	VERIFY(ifp->if_datamov == 0);
	VERIFY(ifp->if_drainers == 0);
	VERIFY(ifp->if_suspend == 0);
	ifp->if_refflags &= ~IFRF_READY;
	lck_mtx_unlock(&ifp->if_ref_lock);

	                                  
	ifclassq_teardown(ifp);

	                              
	lck_mtx_lock(&ifp->if_flt_lock);
	if_flt_monitor_enter(ifp);

	LCK_MTX_ASSERT(&ifp->if_flt_lock, LCK_MTX_ASSERT_OWNED);
	fhead = ifp->if_flt_head;
	TAILQ_INIT(&ifp->if_flt_head);

	for (filter = TAILQ_FIRST(&fhead); filter; filter = filter_next) {
		filter_next = TAILQ_NEXT(filter, filt_next);
		lck_mtx_unlock(&ifp->if_flt_lock);

		dlil_detach_filter_internal(filter, 1);
		lck_mtx_lock(&ifp->if_flt_lock);
	}
	if_flt_monitor_leave(ifp);
	lck_mtx_unlock(&ifp->if_flt_lock);

	                                                       
	if_purgeaddrs(ifp);

	ifnet_lock_exclusive(ifp);

	                           
	for (i = 0; i < PROTO_HASH_SLOTS; i++) {
		struct if_proto *proto;

		proto = SLIST_FIRST(&ifp->if_proto_hash[i]);
		while (proto != NULL) {
			protocol_family_t family = proto->protocol_family;
			ifnet_lock_done(ifp);
			proto_unplumb(family, ifp);
			ifnet_lock_exclusive(ifp);
			proto = SLIST_FIRST(&ifp->if_proto_hash[i]);
		}
		                                            
		VERIFY(SLIST_EMPTY(&ifp->if_proto_hash[i]));
	}
	zfree(dlif_phash_zone, ifp->if_proto_hash);
	ifp->if_proto_hash = NULL;

	                                                      
	ifa = TAILQ_FIRST(&ifp->if_addrhead);
	VERIFY(ifnet_addrs[ifp->if_index - 1] == ifa);
	IFA_LOCK(ifa);
	if_detach_link_ifa(ifp, ifa);
	IFA_UNLOCK(ifa);

	                                                        
	IFA_REMREF(ifa);
	ifnet_addrs[ifp->if_index - 1] = NULL;

	                                                            
	VERIFY(ifp->if_link.tqe_next == NULL);
	VERIFY(ifp->if_link.tqe_prev == NULL);
	VERIFY(ifp->if_detaching_link.tqe_next == NULL);
	VERIFY(ifp->if_detaching_link.tqe_prev == NULL);
	VERIFY(ifp->if_ordered_link.tqe_next == NULL);
	VERIFY(ifp->if_ordered_link.tqe_prev == NULL);

	                                       
	VERIFY(ifindex2ifnet[ifp->if_index] == NULL);

	                                            
	VERIFY(TAILQ_EMPTY(&ifp->if_addrhead));

	  
                                                  
    
	if (ifp->if_start_thread != THREAD_NULL) {
		lck_mtx_lock_spin(&ifp->if_start_lock);
		ifp->if_start_flags = 0;
		ifp->if_start_thread = THREAD_NULL;
		wakeup_one((caddr_t)&ifp->if_start_thread);
		lck_mtx_unlock(&ifp->if_start_lock);
	}

	  
                                                 
    
	if (ifp->if_poll_thread != THREAD_NULL) {
		lck_mtx_lock_spin(&ifp->if_poll_lock);
		ifp->if_poll_thread = THREAD_NULL;
		wakeup_one((caddr_t)&ifp->if_poll_thread);
		lck_mtx_unlock(&ifp->if_poll_lock);
	}

	  
                                                                    
                                                                   
                                                                    
                                    
    
	if ((inp = ifp->if_inp) != NULL) {
		VERIFY(inp != dlil_main_input_thread);

		if (inp->dlth_affinity) {
			struct thread *tp, *wtp, *ptp;

			lck_mtx_lock_spin(&inp->dlth_lock);
			wtp = inp->dlth_driver_thread;
			inp->dlth_driver_thread = THREAD_NULL;
			ptp = inp->dlth_poller_thread;
			inp->dlth_poller_thread = THREAD_NULL;
			ASSERT(inp->dlth_thread != THREAD_NULL);
			tp = inp->dlth_thread;                           
			inp->dlth_affinity_tag = 0;
			inp->dlth_affinity = FALSE;
			lck_mtx_unlock(&inp->dlth_lock);

			                                    
			if (ptp != NULL) {
				VERIFY(ifp->if_eflags & IFEF_RXPOLL);
				VERIFY(ifp->if_xflags & IFXF_LEGACY);
				(void) dlil_affinity_set(ptp,
				    THREAD_AFFINITY_TAG_NULL);
				thread_deallocate(ptp);
			}

			                                        
			if (wtp != NULL) {
				(void) dlil_affinity_set(wtp,
				    THREAD_AFFINITY_TAG_NULL);
				thread_deallocate(wtp);
			}

			                                          
			(void) dlil_affinity_set(tp, THREAD_AFFINITY_TAG_NULL);
			thread_deallocate(tp);
		}

		                                        
		ifp->if_inp = NULL;

		                                                            
		if (inp->dlth_thread != THREAD_NULL) {
			lck_mtx_lock_spin(&inp->dlth_lock);
			inp->dlth_flags |= DLIL_INPUT_TERMINATE;
			if (!(inp->dlth_flags & DLIL_INPUT_RUNNING)) {
				wakeup_one((caddr_t)&inp->dlth_flags);
			}
			lck_mtx_unlock(&inp->dlth_lock);
			ifnet_lock_done(ifp);

			                                            
			lck_mtx_lock_spin(&inp->dlth_lock);
			while ((inp->dlth_flags & DLIL_INPUT_TERMINATE_COMPLETE)
			    == 0) {
				(void) msleep(&inp->dlth_flags, &inp->dlth_lock,
				    (PZERO - 1) | PSPIN, inp->dlth_name, NULL);
			}
			lck_mtx_unlock(&inp->dlth_lock);
			ifnet_lock_exclusive(ifp);
		}

		                                 
		dlil_clean_threading_info(inp);
		                              
		VERIFY(ifp->if_poll_thread == THREAD_NULL);
		dlil_reset_rxpoll_params(ifp);
	}

	                                                          
	if_free = ifp->if_free;
	ifp->if_output_dlil = ifp_if_output;
	ifp->if_output = ifp_if_output;
	ifp->if_pre_enqueue = ifp_if_output;
	ifp->if_start = ifp_if_start;
	ifp->if_output_ctl = ifp_if_ctl;
	ifp->if_input_dlil = ifp_if_input;
	ifp->if_input_poll = ifp_if_input_poll;
	ifp->if_input_ctl = ifp_if_ctl;
	ifp->if_ioctl = ifp_if_ioctl;
	ifp->if_set_bpf_tap = ifp_if_set_bpf_tap;
	ifp->if_free = ifp_if_free;
	ifp->if_demux = ifp_if_demux;
	ifp->if_event = ifp_if_event;
	ifp->if_framer_legacy = ifp_if_framer;
	ifp->if_framer = ifp_if_framer_extended;
	ifp->if_add_proto = ifp_if_add_proto;
	ifp->if_del_proto = ifp_if_del_proto;
	ifp->if_check_multi = ifp_if_check_multi;

	                                    
	VERIFY(ifp->if_desc.ifd_maxlen == IF_DESCSIZE);
	ifp->if_desc.ifd_len = 0;
	VERIFY(ifp->if_desc.ifd_desc != NULL);
	bzero(ifp->if_desc.ifd_desc, IF_DESCSIZE);

	                                              
	VERIFY(ifp->if_delegated.ifp == NULL);
	VERIFY(ifp->if_delegated.type == 0);
	VERIFY(ifp->if_delegated.family == 0);
	VERIFY(ifp->if_delegated.subfamily == 0);
	VERIFY(ifp->if_delegated.expensive == 0);
	VERIFY(ifp->if_delegated.constrained == 0);

	                             
	if_clear_eflags(ifp, IFEF_QOSMARKING_ENABLED);
	if_set_qosmarking_mode(ifp, IFRTYPE_QOSMARKING_MODE_NONE);


	ifnet_lock_done(ifp);

#if PF
	  
                                                         
    
	pf_ifnet_hook(ifp, 0);
#endif         

	                                 
	lck_mtx_lock_spin(&ifp->if_flt_lock);
	VERIFY(TAILQ_EMPTY(&ifp->if_flt_head));
	VERIFY(ifp->if_flt_busy == 0);
	VERIFY(ifp->if_flt_waiters == 0);
	lck_mtx_unlock(&ifp->if_flt_lock);

	                                     
	if_qflush(ifp, 0);

	                                             
	lck_mtx_lock(&ifp->if_cached_route_lock);
	VERIFY(!ifp->if_fwd_cacheok);
	ROUTE_RELEASE(&ifp->if_fwd_route);
	bzero(&ifp->if_fwd_route, sizeof(ifp->if_fwd_route));
	ROUTE_RELEASE(&ifp->if_src_route);
	bzero(&ifp->if_src_route, sizeof(ifp->if_src_route));
	ROUTE_RELEASE(&ifp->if_src_route6);
	bzero(&ifp->if_src_route6, sizeof(ifp->if_src_route6));
	lck_mtx_unlock(&ifp->if_cached_route_lock);

	VERIFY(ifp->if_data_threshold == 0);
	VERIFY(ifp->if_dt_tcall != NULL);
	VERIFY(!thread_call_isactive(ifp->if_dt_tcall));

	ifnet_llreach_ifdetach(ifp);

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_IF_DETACHED, NULL, 0);

	  
                                         
    
	lck_mtx_lock_spin(&ifp->if_ref_lock);
	if (!(ifp->if_refflags & IFRF_DETACHING)) {
		panic("%s: flags mismatch (detaching not set) ifp=%p",
		    __func__, ifp);
		                
	}
	ifp->if_refflags &= ~IFRF_DETACHING;
	lck_mtx_unlock(&ifp->if_ref_lock);
	if (if_free != NULL) {
		if_free(ifp);
	}

	if (dlil_verbose) {
		DLIL_PRINTF("%s: detached\n", if_name(ifp));
	}

	                                                
	ifnet_release(ifp);
}

errno_t
ifp_if_output(struct ifnet *ifp, struct mbuf *m)
{
#pragma unused(ifp)
	m_freem_list(m);
	return 0;
}

void
ifp_if_start(struct ifnet *ifp)
{
	ifnet_purge(ifp);
}

static errno_t
ifp_if_input(struct ifnet *ifp, struct mbuf *m_head,
    struct mbuf *m_tail, const struct ifnet_stat_increment_param *s,
    boolean_t poll, struct thread *tp)
{
#pragma unused(ifp, m_tail, s, poll, tp)
	m_freem_list(m_head);
	return ENXIO;
}

static void
ifp_if_input_poll(struct ifnet *ifp, u_int32_t flags, u_int32_t max_cnt,
    struct mbuf **m_head, struct mbuf **m_tail, u_int32_t *cnt, u_int32_t *len)
{
#pragma unused(ifp, flags, max_cnt)
	if (m_head != NULL) {
		*m_head = NULL;
	}
	if (m_tail != NULL) {
		*m_tail = NULL;
	}
	if (cnt != NULL) {
		*cnt = 0;
	}
	if (len != NULL) {
		*len = 0;
	}
}

static errno_t
ifp_if_ctl(struct ifnet *ifp, ifnet_ctl_cmd_t cmd, u_int32_t arglen, void *arg)
{
#pragma unused(ifp, cmd, arglen, arg)
	return EOPNOTSUPP;
}

static errno_t
ifp_if_demux(struct ifnet *ifp, struct mbuf *m, char *fh, protocol_family_t *pf)
{
#pragma unused(ifp, fh, pf)
	m_freem(m);
	return EJUSTRETURN;
}

static errno_t
ifp_if_add_proto(struct ifnet *ifp, protocol_family_t pf,
    const struct ifnet_demux_desc *da, u_int32_t dc)
{
#pragma unused(ifp, pf, da, dc)
	return EINVAL;
}

static errno_t
ifp_if_del_proto(struct ifnet *ifp, protocol_family_t pf)
{
#pragma unused(ifp, pf)
	return EINVAL;
}

static errno_t
ifp_if_check_multi(struct ifnet *ifp, const struct sockaddr *sa)
{
#pragma unused(ifp, sa)
	return EOPNOTSUPP;
}

#if !XNU_TARGET_OS_OSX
static errno_t
ifp_if_framer(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *sa, const char *ll, const char *t,
    u_int32_t *pre, u_int32_t *post)
#else                        
static errno_t
ifp_if_framer(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *sa, const char *ll, const char *t)
#endif                        
{
#pragma unused(ifp, m, sa, ll, t)
#if !XNU_TARGET_OS_OSX
	return ifp_if_framer_extended(ifp, m, sa, ll, t, pre, post);
#else                        
	return ifp_if_framer_extended(ifp, m, sa, ll, t, NULL, NULL);
#endif                        
}

static errno_t
ifp_if_framer_extended(struct ifnet *ifp, struct mbuf **m,
    const struct sockaddr *sa, const char *ll, const char *t,
    u_int32_t *pre, u_int32_t *post)
{
#pragma unused(ifp, sa, ll, t)
	m_freem(*m);
	*m = NULL;

	if (pre != NULL) {
		*pre = 0;
	}
	if (post != NULL) {
		*post = 0;
	}

	return EJUSTRETURN;
}

errno_t
ifp_if_ioctl(struct ifnet *ifp, unsigned long cmd, void *arg)
{
#pragma unused(ifp, cmd, arg)
	return EOPNOTSUPP;
}

static errno_t
ifp_if_set_bpf_tap(struct ifnet *ifp, bpf_tap_mode tm, bpf_packet_func f)
{
#pragma unused(ifp, tm, f)
	                                  
	return 0;
}

static void
ifp_if_free(struct ifnet *ifp)
{
#pragma unused(ifp)
}

static void
ifp_if_event(struct ifnet *ifp, const struct kev_msg *e)
{
#pragma unused(ifp, e)
}

int
dlil_if_acquire(u_int32_t family, const void *uniqueid,
    size_t uniqueid_len, const char *ifxname, struct ifnet **ifp)
{
	struct ifnet *ifp1 = NULL;
	struct dlil_ifnet *dlifp1 = NULL;
	struct dlil_ifnet *dlifp1_saved = NULL;
	void *buf, *base, **pbuf;
	int ret = 0;

	VERIFY(*ifp == NULL);
	dlil_if_lock();
	  
                                                            
                    
                                                            
    
	TAILQ_FOREACH(dlifp1, &dlil_ifnet_head, dl_if_link) {
		ifp1 = (struct ifnet *)dlifp1;

		if (ifp1->if_family != family) {
			continue;
		}

		  
                                                             
                                             
     
		lck_mtx_lock(&dlifp1->dl_if_lock);
		if (strncmp(ifxname, ifp1->if_xname, IFXNAMSIZ) == 0) {
			if (dlifp1->dl_if_flags & DLIF_INUSE) {
				lck_mtx_unlock(&dlifp1->dl_if_lock);
				ret = EBUSY;
				goto end;
			}
		}

		if (uniqueid_len) {
			if (uniqueid_len == dlifp1->dl_if_uniqueid_len &&
			    bcmp(uniqueid, dlifp1->dl_if_uniqueid, uniqueid_len) == 0) {
				if (dlifp1->dl_if_flags & DLIF_INUSE) {
					lck_mtx_unlock(&dlifp1->dl_if_lock);
					ret = EBUSY;
					goto end;
				} else {
					                                                    
					if (*ifp == NULL) {
						*ifp = ifp1;
						dlifp1_saved = dlifp1;
					}
					  
                                                              
                                                             
        
				}
			}
		}
		lck_mtx_unlock(&dlifp1->dl_if_lock);
	}

	                                                            
	if (*ifp != NULL) {
		if (dlifp1_saved != NULL) {
			lck_mtx_lock(&dlifp1_saved->dl_if_lock);
			dlifp1_saved->dl_if_flags |= (DLIF_INUSE | DLIF_REUSE);
			lck_mtx_unlock(&dlifp1_saved->dl_if_lock);
			dlifp1_saved = NULL;
		}
		goto end;
	}

	                                            
	buf = zalloc_flags(dlif_zone, Z_WAITOK | Z_ZERO);
	if (buf == NULL) {
		ret = ENOMEM;
		goto end;
	}

	                                                         
	base = (void *)P2ROUNDUP((intptr_t)buf + sizeof(u_int64_t),
	    sizeof(u_int64_t));
	VERIFY(((intptr_t)base + dlif_size) <= ((intptr_t)buf + dlif_bufsize));

	  
                                                      
                                                      
    
	pbuf = (void **)((intptr_t)base - sizeof(void *));
	*pbuf = buf;
	dlifp1 = base;

	if (uniqueid_len) {
		MALLOC(dlifp1->dl_if_uniqueid, void *, uniqueid_len,
		    M_NKE, M_WAITOK);
		if (dlifp1->dl_if_uniqueid == NULL) {
			zfree(dlif_zone, buf);
			ret = ENOMEM;
			goto end;
		}
		bcopy(uniqueid, dlifp1->dl_if_uniqueid, uniqueid_len);
		dlifp1->dl_if_uniqueid_len = uniqueid_len;
	}

	ifp1 = (struct ifnet *)dlifp1;
	dlifp1->dl_if_flags = DLIF_INUSE;
	if (ifnet_debug) {
		dlifp1->dl_if_flags |= DLIF_DEBUG;
		dlifp1->dl_if_trace = dlil_if_trace;
	}
	ifp1->if_name = dlifp1->dl_if_namestorage;
	ifp1->if_xname = dlifp1->dl_if_xnamestorage;

	                                      
	ifp1->if_desc.ifd_maxlen = IF_DESCSIZE;
	ifp1->if_desc.ifd_len = 0;
	ifp1->if_desc.ifd_desc = dlifp1->dl_if_descstorage;


	if ((ret = dlil_alloc_local_stats(ifp1)) != 0) {
		DLIL_PRINTF("%s: failed to allocate if local stats, "
		    "error: %d\n", __func__, ret);
		                                      
		ret = 0;
	}

	lck_mtx_init(&dlifp1->dl_if_lock, ifnet_lock_group, ifnet_lock_attr);
	lck_rw_init(&ifp1->if_lock, ifnet_lock_group, ifnet_lock_attr);
	lck_mtx_init(&ifp1->if_ref_lock, ifnet_lock_group, ifnet_lock_attr);
	lck_mtx_init(&ifp1->if_flt_lock, ifnet_lock_group, ifnet_lock_attr);
	lck_mtx_init(&ifp1->if_addrconfig_lock, ifnet_lock_group,
	    ifnet_lock_attr);
	lck_rw_init(&ifp1->if_llreach_lock, ifnet_lock_group, ifnet_lock_attr);
#if INET
	lck_rw_init(&ifp1->if_inetdata_lock, ifnet_lock_group,
	    ifnet_lock_attr);
	ifp1->if_inetdata = NULL;
#endif
	lck_rw_init(&ifp1->if_inet6data_lock, ifnet_lock_group,
	    ifnet_lock_attr);
	ifp1->if_inet6data = NULL;
	lck_rw_init(&ifp1->if_link_status_lock, ifnet_lock_group,
	    ifnet_lock_attr);
	ifp1->if_link_status = NULL;

	                         
	lck_mtx_init(&ifp1->if_start_lock, ifnet_snd_lock_group,
	    ifnet_lock_attr);
	lck_mtx_init(&ifp1->if_cached_route_lock, ifnet_snd_lock_group,
	    ifnet_lock_attr);
	lck_mtx_init(&ifp1->if_snd.ifcq_lock, ifnet_snd_lock_group,
	    ifnet_lock_attr);

	                            
	lck_mtx_init(&ifp1->if_poll_lock, ifnet_rcv_lock_group,
	    ifnet_lock_attr);

	                                                         
	ifp1->if_dt_tcall = thread_call_allocate_with_options(dlil_dt_tcall_fn,
	    ifp1, THREAD_CALL_PRIORITY_KERNEL, THREAD_CALL_OPTIONS_ONCE);
	if (ifp1->if_dt_tcall == NULL) {
		panic_plain("%s: couldn't create if_dt_tcall", __func__);
		                
	}

	TAILQ_INSERT_TAIL(&dlil_ifnet_head, dlifp1, dl_if_link);

	*ifp = ifp1;

end:
	dlil_if_unlock();

	VERIFY(dlifp1 == NULL || (IS_P2ALIGNED(dlifp1, sizeof(u_int64_t)) &&
	    IS_P2ALIGNED(&ifp1->if_data, sizeof(u_int64_t))));

	return ret;
}

__private_extern__ void
dlil_if_release(ifnet_t ifp)
{
	struct dlil_ifnet *dlifp = (struct dlil_ifnet *)ifp;

	VERIFY(OSDecrementAtomic64(&net_api_stats.nas_ifnet_alloc_count) > 0);
	if (!(ifp->if_xflags & IFXF_ALLOC_KPI)) {
		VERIFY(OSDecrementAtomic64(&net_api_stats.nas_ifnet_alloc_os_count) > 0);
	}

	ifnet_lock_exclusive(ifp);
	lck_mtx_lock(&dlifp->dl_if_lock);
	dlifp->dl_if_flags &= ~DLIF_INUSE;
	strlcpy(dlifp->dl_if_namestorage, ifp->if_name, IFNAMSIZ);
	ifp->if_name = dlifp->dl_if_namestorage;
	                                       
	ifp->if_xname = dlifp->dl_if_xnamestorage;
	snprintf(__DECONST(char *, ifp->if_xname), IFXNAMSIZ,
	    "%s?", ifp->if_name);
	lck_mtx_unlock(&dlifp->dl_if_lock);
	ifnet_lock_done(ifp);
}

__private_extern__ void
dlil_if_lock(void)
{
	lck_mtx_lock(&dlil_ifnet_lock);
}

__private_extern__ void
dlil_if_unlock(void)
{
	lck_mtx_unlock(&dlil_ifnet_lock);
}

__private_extern__ void
dlil_if_lock_assert(void)
{
	LCK_MTX_ASSERT(&dlil_ifnet_lock, LCK_MTX_ASSERT_OWNED);
}

__private_extern__ void
dlil_proto_unplumb_all(struct ifnet *ifp)
{
	  
                                                                   
                                                                    
                     
   
                                                               
                                                                 
                                                  
    
	(void) proto_unplumb(PF_INET, ifp);
	(void) proto_unplumb(PF_INET6, ifp);
}

static void
ifp_src_route_copyout(struct ifnet *ifp, struct route *dst)
{
	lck_mtx_lock_spin(&ifp->if_cached_route_lock);
	lck_mtx_convert_spin(&ifp->if_cached_route_lock);

	route_copyout(dst, &ifp->if_src_route, sizeof(*dst));

	lck_mtx_unlock(&ifp->if_cached_route_lock);
}

static void
ifp_src_route_copyin(struct ifnet *ifp, struct route *src)
{
	lck_mtx_lock_spin(&ifp->if_cached_route_lock);
	lck_mtx_convert_spin(&ifp->if_cached_route_lock);

	if (ifp->if_fwd_cacheok) {
		route_copyin(src, &ifp->if_src_route, sizeof(*src));
	} else {
		ROUTE_RELEASE(src);
	}
	lck_mtx_unlock(&ifp->if_cached_route_lock);
}

static void
ifp_src_route6_copyout(struct ifnet *ifp, struct route_in6 *dst)
{
	lck_mtx_lock_spin(&ifp->if_cached_route_lock);
	lck_mtx_convert_spin(&ifp->if_cached_route_lock);

	route_copyout((struct route *)dst, (struct route *)&ifp->if_src_route6,
	    sizeof(*dst));

	lck_mtx_unlock(&ifp->if_cached_route_lock);
}

static void
ifp_src_route6_copyin(struct ifnet *ifp, struct route_in6 *src)
{
	lck_mtx_lock_spin(&ifp->if_cached_route_lock);
	lck_mtx_convert_spin(&ifp->if_cached_route_lock);

	if (ifp->if_fwd_cacheok) {
		route_copyin((struct route *)src,
		    (struct route *)&ifp->if_src_route6, sizeof(*src));
	} else {
		ROUTE_RELEASE(src);
	}
	lck_mtx_unlock(&ifp->if_cached_route_lock);
}

struct rtentry *
ifnet_cached_rtlookup_inet(struct ifnet *ifp, struct in_addr src_ip)
{
	struct route            src_rt;
	struct sockaddr_in      *dst;

	dst = (struct sockaddr_in *)(void *)(&src_rt.ro_dst);

	ifp_src_route_copyout(ifp, &src_rt);

	if (ROUTE_UNUSABLE(&src_rt) || src_ip.s_addr != dst->sin_addr.s_addr) {
		ROUTE_RELEASE(&src_rt);
		if (dst->sin_family != AF_INET) {
			bzero(&src_rt.ro_dst, sizeof(src_rt.ro_dst));
			dst->sin_len = sizeof(src_rt.ro_dst);
			dst->sin_family = AF_INET;
		}
		dst->sin_addr = src_ip;

		VERIFY(src_rt.ro_rt == NULL);
		src_rt.ro_rt = rtalloc1_scoped((struct sockaddr *)dst,
		    0, 0, ifp->if_index);

		if (src_rt.ro_rt != NULL) {
			                                       
			struct rtentry  *rte = src_rt.ro_rt;
			RT_ADDREF(rte);
			ifp_src_route_copyin(ifp, &src_rt);
			src_rt.ro_rt = rte;
		}
	}

	return src_rt.ro_rt;
}

struct rtentry *
ifnet_cached_rtlookup_inet6(struct ifnet *ifp, struct in6_addr *src_ip6)
{
	struct route_in6 src_rt;

	ifp_src_route6_copyout(ifp, &src_rt);

	if (ROUTE_UNUSABLE(&src_rt) ||
	    !IN6_ARE_ADDR_EQUAL(src_ip6, &src_rt.ro_dst.sin6_addr)) {
		ROUTE_RELEASE(&src_rt);
		if (src_rt.ro_dst.sin6_family != AF_INET6) {
			bzero(&src_rt.ro_dst, sizeof(src_rt.ro_dst));
			src_rt.ro_dst.sin6_len = sizeof(src_rt.ro_dst);
			src_rt.ro_dst.sin6_family = AF_INET6;
		}
		src_rt.ro_dst.sin6_scope_id = in6_addr2scopeid(ifp, src_ip6);
		bcopy(src_ip6, &src_rt.ro_dst.sin6_addr,
		    sizeof(src_rt.ro_dst.sin6_addr));

		if (src_rt.ro_rt == NULL) {
			src_rt.ro_rt = rtalloc1_scoped(
				(struct sockaddr *)&src_rt.ro_dst, 0, 0,
				ifp->if_index);

			if (src_rt.ro_rt != NULL) {
				                                       
				struct rtentry  *rte = src_rt.ro_rt;
				RT_ADDREF(rte);
				ifp_src_route6_copyin(ifp, &src_rt);
				src_rt.ro_rt = rte;
			}
		}
	}

	return src_rt.ro_rt;
}

void
if_lqm_update(struct ifnet *ifp, int lqm, int locked)
{
	struct kev_dl_link_quality_metric_data ev_lqm_data;

	VERIFY(lqm >= IFNET_LQM_MIN && lqm <= IFNET_LQM_MAX);

	                       
	if (lqm >= 0 && lqm <= IFNET_LQM_THRESH_ABORT) {
		lqm = IFNET_LQM_THRESH_ABORT;
		atomic_bitset_32(&tcbinfo.ipi_flags,
		    INPCBINFO_HANDLE_LQM_ABORT);
		inpcb_timer_sched(&tcbinfo, INPCB_TIMER_FAST);
	} else if (lqm > IFNET_LQM_THRESH_ABORT &&
	    lqm <= IFNET_LQM_THRESH_MINIMALLY_VIABLE) {
		lqm = IFNET_LQM_THRESH_MINIMALLY_VIABLE;
	} else if (lqm > IFNET_LQM_THRESH_MINIMALLY_VIABLE &&
	    lqm <= IFNET_LQM_THRESH_POOR) {
		lqm = IFNET_LQM_THRESH_POOR;
	} else if (lqm > IFNET_LQM_THRESH_POOR &&
	    lqm <= IFNET_LQM_THRESH_GOOD) {
		lqm = IFNET_LQM_THRESH_GOOD;
	}

	  
                           
    
	if (!locked) {
		ifnet_lock_exclusive(ifp);
	}

	if (lqm == ifp->if_interface_state.lqm_state &&
	    (ifp->if_interface_state.valid_bitmask &
	    IF_INTERFACE_STATE_LQM_STATE_VALID)) {
		  
                                                   
     
		if (!locked) {
			ifnet_lock_done(ifp);
		}
		return;                                
	}
	ifp->if_interface_state.valid_bitmask |=
	    IF_INTERFACE_STATE_LQM_STATE_VALID;
	ifp->if_interface_state.lqm_state = (int8_t)lqm;

	  
                                                          
    
	ifnet_lock_done(ifp);

	bzero(&ev_lqm_data, sizeof(ev_lqm_data));
	ev_lqm_data.link_quality_metric = lqm;

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_LINK_QUALITY_METRIC_CHANGED,
	    (struct net_event_data *)&ev_lqm_data, sizeof(ev_lqm_data));

	  
                                     
    
	if (locked) {
		ifnet_lock_exclusive(ifp);
	}
}

static void
if_rrc_state_update(struct ifnet *ifp, unsigned int rrc_state)
{
	struct kev_dl_rrc_state kev;

	if (rrc_state == ifp->if_interface_state.rrc_state &&
	    (ifp->if_interface_state.valid_bitmask &
	    IF_INTERFACE_STATE_RRC_STATE_VALID)) {
		return;
	}

	ifp->if_interface_state.valid_bitmask |=
	    IF_INTERFACE_STATE_RRC_STATE_VALID;

	ifp->if_interface_state.rrc_state = (uint8_t)rrc_state;

	  
                                                          
    
	ifnet_lock_done(ifp);

	bzero(&kev, sizeof(struct kev_dl_rrc_state));
	kev.rrc_state = rrc_state;

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_RRC_STATE_CHANGED,
	    (struct net_event_data *)&kev, sizeof(struct kev_dl_rrc_state));

	ifnet_lock_exclusive(ifp);
}

errno_t
if_state_update(struct ifnet *ifp,
    struct if_interface_state *if_interface_state)
{
	u_short if_index_available = 0;

	ifnet_lock_exclusive(ifp);

	if ((ifp->if_type != IFT_CELLULAR) &&
	    (if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_RRC_STATE_VALID)) {
		ifnet_lock_done(ifp);
		return ENOTSUP;
	}
	if ((if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_LQM_STATE_VALID) &&
	    (if_interface_state->lqm_state < IFNET_LQM_MIN ||
	    if_interface_state->lqm_state > IFNET_LQM_MAX)) {
		ifnet_lock_done(ifp);
		return EINVAL;
	}
	if ((if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_RRC_STATE_VALID) &&
	    if_interface_state->rrc_state !=
	    IF_INTERFACE_STATE_RRC_STATE_IDLE &&
	    if_interface_state->rrc_state !=
	    IF_INTERFACE_STATE_RRC_STATE_CONNECTED) {
		ifnet_lock_done(ifp);
		return EINVAL;
	}

	if (if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_LQM_STATE_VALID) {
		if_lqm_update(ifp, if_interface_state->lqm_state, 1);
	}
	if (if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_RRC_STATE_VALID) {
		if_rrc_state_update(ifp, if_interface_state->rrc_state);
	}
	if (if_interface_state->valid_bitmask &
	    IF_INTERFACE_STATE_INTERFACE_AVAILABILITY_VALID) {
		ifp->if_interface_state.valid_bitmask |=
		    IF_INTERFACE_STATE_INTERFACE_AVAILABILITY_VALID;
		ifp->if_interface_state.interface_availability =
		    if_interface_state->interface_availability;

		if (ifp->if_interface_state.interface_availability ==
		    IF_INTERFACE_STATE_INTERFACE_AVAILABLE) {
			os_log(OS_LOG_DEFAULT, "%s: interface %s (%u) available\n",
			    __func__, if_name(ifp), ifp->if_index);
			if_index_available = ifp->if_index;
		} else {
			os_log(OS_LOG_DEFAULT, "%s: interface %s (%u) unavailable)\n",
			    __func__, if_name(ifp), ifp->if_index);
		}
	}
	ifnet_lock_done(ifp);

	  
                                                                  
                                                                  
                                                             
                                                                 
    
	if (if_index_available > 0) {
		tcp_interface_send_probe(if_index_available);
	}

	return 0;
}

void
if_get_state(struct ifnet *ifp,
    struct if_interface_state *if_interface_state)
{
	ifnet_lock_shared(ifp);

	if_interface_state->valid_bitmask = 0;

	if (ifp->if_interface_state.valid_bitmask &
	    IF_INTERFACE_STATE_RRC_STATE_VALID) {
		if_interface_state->valid_bitmask |=
		    IF_INTERFACE_STATE_RRC_STATE_VALID;
		if_interface_state->rrc_state =
		    ifp->if_interface_state.rrc_state;
	}
	if (ifp->if_interface_state.valid_bitmask &
	    IF_INTERFACE_STATE_LQM_STATE_VALID) {
		if_interface_state->valid_bitmask |=
		    IF_INTERFACE_STATE_LQM_STATE_VALID;
		if_interface_state->lqm_state =
		    ifp->if_interface_state.lqm_state;
	}
	if (ifp->if_interface_state.valid_bitmask &
	    IF_INTERFACE_STATE_INTERFACE_AVAILABILITY_VALID) {
		if_interface_state->valid_bitmask |=
		    IF_INTERFACE_STATE_INTERFACE_AVAILABILITY_VALID;
		if_interface_state->interface_availability =
		    ifp->if_interface_state.interface_availability;
	}

	ifnet_lock_done(ifp);
}

errno_t
if_probe_connectivity(struct ifnet *ifp, u_int32_t conn_probe)
{
	if (conn_probe > 1) {
		return EINVAL;
	}
	if (conn_probe == 0) {
		if_clear_eflags(ifp, IFEF_PROBE_CONNECTIVITY);
	} else {
		if_set_eflags(ifp, IFEF_PROBE_CONNECTIVITY);
	}

#if NECP
	necp_update_all_clients();
#endif           

	tcp_probe_connectivity(ifp, conn_probe);
	return 0;
}

                
static int
get_ether_index(int * ret_other_index)
{
	struct ifnet *ifp;
	int en0_index = 0;
	int other_en_index = 0;
	int any_ether_index = 0;
	short best_unit = 0;

	*ret_other_index = 0;
	TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
		  
                                                             
                       
     
		ifnet_lock_shared(ifp);
		if (strcmp(ifp->if_name, "en") == 0) {
			if (ifp->if_unit == 0) {
				                           
				en0_index = ifp->if_index;
				ifnet_lock_done(ifp);
				break;
			}
			if (other_en_index == 0 || ifp->if_unit < best_unit) {
				other_en_index = ifp->if_index;
				best_unit = ifp->if_unit;
			}
		} else if (ifp->if_type == IFT_ETHER && any_ether_index == 0) {
			any_ether_index = ifp->if_index;
		}
		ifnet_lock_done(ifp);
	}
	if (en0_index == 0) {
		if (other_en_index != 0) {
			*ret_other_index = other_en_index;
		} else if (any_ether_index != 0) {
			*ret_other_index = any_ether_index;
		}
	}
	return en0_index;
}

int
uuid_get_ethernet(u_int8_t *node)
{
	static int en0_index;
	struct ifnet *ifp;
	int other_index = 0;
	int the_index = 0;
	int ret;

	ifnet_head_lock_shared();
	if (en0_index == 0 || ifindex2ifnet[en0_index] == NULL) {
		en0_index = get_ether_index(&other_index);
	}
	if (en0_index != 0) {
		the_index = en0_index;
	} else if (other_index != 0) {
		the_index = other_index;
	}
	if (the_index != 0) {
		struct dlil_ifnet *dl_if;

		ifp = ifindex2ifnet[the_index];
		VERIFY(ifp != NULL);
		dl_if = (struct dlil_ifnet *)ifp;
		if (dl_if->dl_if_permanent_ether_is_set != 0) {
			  
                                                 
                                             
      
			memcpy(node, dl_if->dl_if_permanent_ether,
			    ETHER_ADDR_LEN);
		} else {
			memcpy(node, IF_LLADDR(ifp), ETHER_ADDR_LEN);
		}
		ret = 0;
	} else {
		ret = -1;
	}
	ifnet_head_done();
	return ret;
}

static int
sysctl_rxpoll SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint32_t i;
	int err;

	i = if_rxpoll;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (net_rxpoll == 0) {
		return ENXIO;
	}

	if_rxpoll = i;
	return err;
}

static int
sysctl_rxpoll_mode_holdtime SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint64_t q;
	int err;

	q = if_rxpoll_mode_holdtime;

	err = sysctl_handle_quad(oidp, &q, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (q < IF_RXPOLL_MODE_HOLDTIME_MIN) {
		q = IF_RXPOLL_MODE_HOLDTIME_MIN;
	}

	if_rxpoll_mode_holdtime = q;

	return err;
}

static int
sysctl_rxpoll_sample_holdtime SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint64_t q;
	int err;

	q = if_rxpoll_sample_holdtime;

	err = sysctl_handle_quad(oidp, &q, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (q < IF_RXPOLL_SAMPLETIME_MIN) {
		q = IF_RXPOLL_SAMPLETIME_MIN;
	}

	if_rxpoll_sample_holdtime = q;

	return err;
}

static int
sysctl_rxpoll_interval_time SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint64_t q;
	int err;

	q = if_rxpoll_interval_time;

	err = sysctl_handle_quad(oidp, &q, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (q < IF_RXPOLL_INTERVALTIME_MIN) {
		q = IF_RXPOLL_INTERVALTIME_MIN;
	}

	if_rxpoll_interval_time = q;

	return err;
}

static int
sysctl_rxpoll_wlowat SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint32_t i;
	int err;

	i = if_sysctl_rxpoll_wlowat;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (i == 0 || i >= if_sysctl_rxpoll_whiwat) {
		return EINVAL;
	}

	if_sysctl_rxpoll_wlowat = i;
	return err;
}

static int
sysctl_rxpoll_whiwat SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint32_t i;
	int err;

	i = if_sysctl_rxpoll_whiwat;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (i <= if_sysctl_rxpoll_wlowat) {
		return EINVAL;
	}

	if_sysctl_rxpoll_whiwat = i;
	return err;
}

static int
sysctl_sndq_maxlen SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int i, err;

	i = if_sndq_maxlen;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (i < IF_SNDQ_MINLEN) {
		i = IF_SNDQ_MINLEN;
	}

	if_sndq_maxlen = i;
	return err;
}

static int
sysctl_rcvq_maxlen SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int i, err;

	i = if_rcvq_maxlen;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (i < IF_RCVQ_MINLEN) {
		i = IF_RCVQ_MINLEN;
	}

	if_rcvq_maxlen = i;
	return err;
}

int
dlil_node_present(struct ifnet *ifp, struct sockaddr *sa,
    int32_t rssi, int lqm, int npm, u_int8_t srvinfo[48])
{
	struct kev_dl_node_presence kev;
	struct sockaddr_dl *sdl;
	struct sockaddr_in6 *sin6;
	int ret = 0;

	VERIFY(ifp);
	VERIFY(sa);
	VERIFY(sa->sa_family == AF_LINK || sa->sa_family == AF_INET6);

	bzero(&kev, sizeof(kev));
	sin6 = &kev.sin6_node_address;
	sdl = &kev.sdl_node_address;
	nd6_alt_node_addr_decompose(ifp, sa, sdl, sin6);
	kev.rssi = rssi;
	kev.link_quality_metric = lqm;
	kev.node_proximity_metric = npm;
	bcopy(srvinfo, kev.node_service_info, sizeof(kev.node_service_info));

	ret = nd6_alt_node_present(ifp, sin6, sdl, rssi, lqm, npm);
	if (ret == 0) {
		int err = dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_NODE_PRESENCE,
		    &kev.link_data, sizeof(kev));
		if (err != 0) {
			log(LOG_ERR, "%s: Post DL_NODE_PRESENCE failed with"
			    "error %d\n", __func__, err);
		}
	}
	return ret;
}

void
dlil_node_absent(struct ifnet *ifp, struct sockaddr *sa)
{
	struct kev_dl_node_absence kev = {};
	struct sockaddr_in6 *kev_sin6 = NULL;
	struct sockaddr_dl *kev_sdl = NULL;

	VERIFY(ifp != NULL);
	VERIFY(sa != NULL);
	VERIFY(sa->sa_family == AF_LINK || sa->sa_family == AF_INET6);

	kev_sin6 = &kev.sin6_node_address;
	kev_sdl = &kev.sdl_node_address;

	if (sa->sa_family == AF_INET6) {
		  
                                                 
                                                       
     
		VERIFY(sa->sa_len <= sizeof(*kev_sin6));
		bcopy(sa, kev_sin6, sa->sa_len);
		nd6_alt_node_absent(ifp, kev_sin6, kev_sdl);
	} else {
		  
                                                          
                               
     
		nd6_alt_node_addr_decompose(ifp, sa, kev_sdl, kev_sin6);
		nd6_alt_node_absent(ifp, kev_sin6, NULL);
	}

	kev_sdl->sdl_type = ifp->if_type;
	kev_sdl->sdl_index = ifp->if_index;

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_NODE_ABSENCE,
	    &kev.link_data, sizeof(kev));
}

int
dlil_node_present_v2(struct ifnet *ifp, struct sockaddr *sa, struct sockaddr_dl *sdl,
    int32_t rssi, int lqm, int npm, u_int8_t srvinfo[48])
{
	struct kev_dl_node_presence kev = {};
	struct sockaddr_dl *kev_sdl = NULL;
	struct sockaddr_in6 *kev_sin6 = NULL;
	int ret = 0;

	VERIFY(ifp != NULL);
	VERIFY(sa != NULL && sdl != NULL);
	VERIFY(sa->sa_family == AF_INET6 && sdl->sdl_family == AF_LINK);

	kev_sin6 = &kev.sin6_node_address;
	kev_sdl = &kev.sdl_node_address;

	VERIFY(sdl->sdl_len <= sizeof(*kev_sdl));
	bcopy(sdl, kev_sdl, sdl->sdl_len);
	kev_sdl->sdl_type = ifp->if_type;
	kev_sdl->sdl_index = ifp->if_index;

	VERIFY(sa->sa_len <= sizeof(*kev_sin6));
	bcopy(sa, kev_sin6, sa->sa_len);

	kev.rssi = rssi;
	kev.link_quality_metric = lqm;
	kev.node_proximity_metric = npm;
	bcopy(srvinfo, kev.node_service_info, sizeof(kev.node_service_info));

	ret = nd6_alt_node_present(ifp, SIN6(sa), sdl, rssi, lqm, npm);
	if (ret == 0) {
		int err = dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_NODE_PRESENCE,
		    &kev.link_data, sizeof(kev));
		if (err != 0) {
			log(LOG_ERR, "%s: Post DL_NODE_PRESENCE failed with error %d\n", __func__, err);
		}
	}
	return ret;
}

const void *
dlil_ifaddr_bytes(const struct sockaddr_dl *sdl, size_t *sizep,
    kauth_cred_t *credp)
{
	const u_int8_t *bytes;
	size_t size;

	bytes = CONST_LLADDR(sdl);
	size = sdl->sdl_alen;

#if CONFIG_MACF
	if (dlil_lladdr_ckreq) {
		switch (sdl->sdl_type) {
		case IFT_ETHER:
		case IFT_IEEE1394:
			break;
		default:
			credp = NULL;
			break;
		}
		;

		if (credp && mac_system_check_info(*credp, "net.link.addr")) {
			static const u_int8_t unspec[FIREWIRE_EUI64_LEN] = {
				[0] = 2
			};

			bytes = unspec;
		}
	}
#else
#pragma unused(credp)
#endif

	if (sizep != NULL) {
		*sizep = size;
	}
	return bytes;
}

void
dlil_report_issues(struct ifnet *ifp, u_int8_t modid[DLIL_MODIDLEN],
    u_int8_t info[DLIL_MODARGLEN])
{
	struct kev_dl_issues kev;
	struct timeval tv;

	VERIFY(ifp != NULL);
	VERIFY(modid != NULL);
	_CASSERT(sizeof(kev.modid) == DLIL_MODIDLEN);
	_CASSERT(sizeof(kev.info) == DLIL_MODARGLEN);

	bzero(&kev, sizeof(kev));

	microtime(&tv);
	kev.timestamp = tv.tv_sec;
	bcopy(modid, &kev.modid, DLIL_MODIDLEN);
	if (info != NULL) {
		bcopy(info, &kev.info, DLIL_MODARGLEN);
	}

	dlil_post_msg(ifp, KEV_DL_SUBCLASS, KEV_DL_ISSUES,
	    &kev.link_data, sizeof(kev));
}

errno_t
ifnet_getset_opportunistic(ifnet_t ifp, u_long cmd, struct ifreq *ifr,
    struct proc *p)
{
	u_int32_t level = IFNET_THROTTLE_OFF;
	errno_t result = 0;

	VERIFY(cmd == SIOCSIFOPPORTUNISTIC || cmd == SIOCGIFOPPORTUNISTIC);

	if (cmd == SIOCSIFOPPORTUNISTIC) {
		  
                                                      
     
		if ((result = proc_suser(p)) != 0) {
			return result;
		}

		if (ifr->ifr_opportunistic.ifo_flags ==
		    IFRIFOF_BLOCK_OPPORTUNISTIC) {
			level = IFNET_THROTTLE_OPPORTUNISTIC;
		} else if (ifr->ifr_opportunistic.ifo_flags == 0) {
			level = IFNET_THROTTLE_OFF;
		} else {
			result = EINVAL;
		}

		if (result == 0) {
			result = ifnet_set_throttle(ifp, level);
		}
	} else if ((result = ifnet_get_throttle(ifp, &level)) == 0) {
		ifr->ifr_opportunistic.ifo_flags = 0;
		if (level == IFNET_THROTTLE_OPPORTUNISTIC) {
			ifr->ifr_opportunistic.ifo_flags |=
			    IFRIFOF_BLOCK_OPPORTUNISTIC;
		}
	}

	  
                                                         
                       
    
	if (result == 0) {
		uint32_t flags = 0;
		flags |= (cmd == SIOCSIFOPPORTUNISTIC) ?
		    INPCB_OPPORTUNISTIC_SETCMD : 0;
		flags |= (level == IFNET_THROTTLE_OPPORTUNISTIC) ?
		    INPCB_OPPORTUNISTIC_THROTTLEON : 0;
		ifr->ifr_opportunistic.ifo_inuse =
		    udp_count_opportunistic(ifp->if_index, flags) +
		    tcp_count_opportunistic(ifp->if_index, flags);
	}

	if (result == EALREADY) {
		result = 0;
	}

	return result;
}

int
ifnet_get_throttle(struct ifnet *ifp, u_int32_t *level)
{
	struct ifclassq *ifq;
	int err = 0;

	if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return ENXIO;
	}

	*level = IFNET_THROTTLE_OFF;

	ifq = &ifp->if_snd;
	IFCQ_LOCK(ifq);
	                                                        
	if (IFCQ_IS_ENABLED(ifq)) {
		cqrq_throttle_t req = { 0, IFNET_THROTTLE_OFF };

		err = fq_if_request_classq(ifq, CLASSQRQ_THROTTLE, &req);
		*level = req.level;
	}
	IFCQ_UNLOCK(ifq);

	return err;
}

int
ifnet_set_throttle(struct ifnet *ifp, u_int32_t level)
{
	struct ifclassq *ifq;
	int err = 0;

	if (!(ifp->if_eflags & IFEF_TXSTART)) {
		return ENXIO;
	}

	ifq = &ifp->if_snd;

	switch (level) {
	case IFNET_THROTTLE_OFF:
	case IFNET_THROTTLE_OPPORTUNISTIC:
		break;
	default:
		return EINVAL;
	}

	IFCQ_LOCK(ifq);
	if (IFCQ_IS_ENABLED(ifq)) {
		cqrq_throttle_t req = { 1, level };

		err = fq_if_request_classq(ifq, CLASSQRQ_THROTTLE, &req);
	}
	IFCQ_UNLOCK(ifq);

	if (err == 0) {
		DLIL_PRINTF("%s: throttling level set to %d\n", if_name(ifp),
		    level);
#if NECP
		necp_update_all_clients();
#endif           
		if (level == IFNET_THROTTLE_OFF) {
			ifnet_start(ifp);
		}
	}

	return err;
}

errno_t
ifnet_getset_log(ifnet_t ifp, u_long cmd, struct ifreq *ifr,
    struct proc *p)
{
#pragma unused(p)
	errno_t result = 0;
	uint32_t flags;
	int level, category, subcategory;

	VERIFY(cmd == SIOCSIFLOG || cmd == SIOCGIFLOG);

	if (cmd == SIOCSIFLOG) {
		if ((result = priv_check_cred(kauth_cred_get(),
		    PRIV_NET_INTERFACE_CONTROL, 0)) != 0) {
			return result;
		}

		level = ifr->ifr_log.ifl_level;
		if (level < IFNET_LOG_MIN || level > IFNET_LOG_MAX) {
			result = EINVAL;
		}

		flags = ifr->ifr_log.ifl_flags;
		if ((flags &= IFNET_LOGF_MASK) == 0) {
			result = EINVAL;
		}

		category = ifr->ifr_log.ifl_category;
		subcategory = ifr->ifr_log.ifl_subcategory;

		if (result == 0) {
			result = ifnet_set_log(ifp, level, flags,
			    category, subcategory);
		}
	} else {
		result = ifnet_get_log(ifp, &level, &flags, &category,
		    &subcategory);
		if (result == 0) {
			ifr->ifr_log.ifl_level = level;
			ifr->ifr_log.ifl_flags = flags;
			ifr->ifr_log.ifl_category = category;
			ifr->ifr_log.ifl_subcategory = subcategory;
		}
	}

	return result;
}

int
ifnet_set_log(struct ifnet *ifp, int32_t level, uint32_t flags,
    int32_t category, int32_t subcategory)
{
	int err = 0;

	VERIFY(level >= IFNET_LOG_MIN && level <= IFNET_LOG_MAX);
	VERIFY(flags & IFNET_LOGF_MASK);

	  
                                                             
                                                
    
	flags |= ifp->if_log.flags;

	if (ifp->if_output_ctl != NULL) {
		struct ifnet_log_params l;

		bzero(&l, sizeof(l));
		l.level = level;
		l.flags = flags;
		l.flags &= ~IFNET_LOGF_DLIL;
		l.category = category;
		l.subcategory = subcategory;

		                                       
		if (l.flags != 0) {
			err = ifp->if_output_ctl(ifp, IFNET_CTL_SET_LOG,
			    sizeof(l), &l);
		}
	} else if ((flags & ~IFNET_LOGF_DLIL) && ifp->if_output_ctl == NULL) {
		  
                                                      
                                                       
                                                
     
		flags &= IFNET_LOGF_DLIL;
		if (flags == 0 && (!(ifp->if_log.flags & IFNET_LOGF_DLIL))) {
			level = 0;
		}
	}

	if (err == 0) {
		if ((ifp->if_log.level = level) == IFNET_LOG_DEFAULT) {
			ifp->if_log.flags = 0;
		} else {
			ifp->if_log.flags |= flags;
		}

		log(LOG_INFO, "%s: logging level set to %d flags=%b "
		    "arg=%b, category=%d subcategory=%d\n", if_name(ifp),
		    ifp->if_log.level, ifp->if_log.flags,
		    IFNET_LOGF_BITS, flags, IFNET_LOGF_BITS,
		    category, subcategory);
	}

	return err;
}

int
ifnet_get_log(struct ifnet *ifp, int32_t *level, uint32_t *flags,
    int32_t *category, int32_t *subcategory)
{
	if (level != NULL) {
		*level = ifp->if_log.level;
	}
	if (flags != NULL) {
		*flags = ifp->if_log.flags;
	}
	if (category != NULL) {
		*category = ifp->if_log.category;
	}
	if (subcategory != NULL) {
		*subcategory = ifp->if_log.subcategory;
	}

	return 0;
}

int
ifnet_notify_address(struct ifnet *ifp, int af)
{
	struct ifnet_notify_address_params na;

#if PF
	(void) pf_ifaddr_hook(ifp);
#endif         

	if (ifp->if_output_ctl == NULL) {
		return EOPNOTSUPP;
	}

	bzero(&na, sizeof(na));
	na.address_family = (sa_family_t)af;

	return ifp->if_output_ctl(ifp, IFNET_CTL_NOTIFY_ADDRESS,
	           sizeof(na), &na);
}

errno_t
ifnet_flowid(struct ifnet *ifp, uint32_t *flowid)
{
	if (ifp == NULL || flowid == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    !IF_FULLY_ATTACHED(ifp)) {
		return ENXIO;
	}

	*flowid = ifp->if_flowhash;

	return 0;
}

errno_t
ifnet_disable_output(struct ifnet *ifp)
{
	int err;

	if (ifp == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    !IF_FULLY_ATTACHED(ifp)) {
		return ENXIO;
	}

	if ((err = ifnet_fc_add(ifp)) == 0) {
		lck_mtx_lock_spin(&ifp->if_start_lock);
		ifp->if_start_flags |= IFSF_FLOW_CONTROLLED;
		lck_mtx_unlock(&ifp->if_start_lock);
	}
	return err;
}

errno_t
ifnet_enable_output(struct ifnet *ifp)
{
	if (ifp == NULL) {
		return EINVAL;
	} else if (!(ifp->if_eflags & IFEF_TXSTART) ||
	    !IF_FULLY_ATTACHED(ifp)) {
		return ENXIO;
	}

	ifnet_start_common(ifp, TRUE);
	return 0;
}

void
ifnet_flowadv(uint32_t flowhash)
{
	struct ifnet_fc_entry *ifce;
	struct ifnet *ifp;

	ifce = ifnet_fc_get(flowhash);
	if (ifce == NULL) {
		return;
	}

	VERIFY(ifce->ifce_ifp != NULL);
	ifp = ifce->ifce_ifp;

	                                                      
	if (ifnet_is_attached(ifp, 1)) {
		if (ifp->if_flowhash == flowhash) {
			(void) ifnet_enable_output(ifp);
		}
		ifnet_decr_iorefcnt(ifp);
	}
	ifnet_fc_entry_free(ifce);
}

  
                                                                  
   
static inline int
ifce_cmp(const struct ifnet_fc_entry *fc1, const struct ifnet_fc_entry *fc2)
{
	return fc1->ifce_flowhash - fc2->ifce_flowhash;
}

static int
ifnet_fc_add(struct ifnet *ifp)
{
	struct ifnet_fc_entry keyfc, *ifce;
	uint32_t flowhash;

	VERIFY(ifp != NULL && (ifp->if_eflags & IFEF_TXSTART));
	VERIFY(ifp->if_flowhash != 0);
	flowhash = ifp->if_flowhash;

	bzero(&keyfc, sizeof(keyfc));
	keyfc.ifce_flowhash = flowhash;

	lck_mtx_lock_spin(&ifnet_fc_lock);
	ifce = RB_FIND(ifnet_fc_tree, &ifnet_fc_tree, &keyfc);
	if (ifce != NULL && ifce->ifce_ifp == ifp) {
		                                               
		lck_mtx_unlock(&ifnet_fc_lock);
		return 0;
	}

	if (ifce != NULL) {
		  
                                                          
                                                         
                                                         
                                                         
     
		lck_mtx_unlock(&ifnet_fc_lock);
		return EAGAIN;
	}

	                          
	lck_mtx_convert_spin(&ifnet_fc_lock);

	ifce = zalloc_flags(ifnet_fc_zone, Z_WAITOK | Z_ZERO);
	ifce->ifce_flowhash = flowhash;
	ifce->ifce_ifp = ifp;

	RB_INSERT(ifnet_fc_tree, &ifnet_fc_tree, ifce);
	lck_mtx_unlock(&ifnet_fc_lock);
	return 0;
}

static struct ifnet_fc_entry *
ifnet_fc_get(uint32_t flowhash)
{
	struct ifnet_fc_entry keyfc, *ifce;
	struct ifnet *ifp;

	bzero(&keyfc, sizeof(keyfc));
	keyfc.ifce_flowhash = flowhash;

	lck_mtx_lock_spin(&ifnet_fc_lock);
	ifce = RB_FIND(ifnet_fc_tree, &ifnet_fc_tree, &keyfc);
	if (ifce == NULL) {
		                                                   
		lck_mtx_unlock(&ifnet_fc_lock);
		return NULL;
	}

	RB_REMOVE(ifnet_fc_tree, &ifnet_fc_tree, ifce);

	VERIFY(ifce->ifce_ifp != NULL);
	ifp = ifce->ifce_ifp;

	                          
	lck_mtx_convert_spin(&ifnet_fc_lock);

	if (!ifnet_is_attached(ifp, 0)) {
		  
                                                        
                                     
     
		ifnet_fc_entry_free(ifce);
		ifce = NULL;
	}
	lck_mtx_unlock(&ifnet_fc_lock);

	return ifce;
}

static void
ifnet_fc_entry_free(struct ifnet_fc_entry *ifce)
{
	zfree(ifnet_fc_zone, ifce);
}

static uint32_t
ifnet_calc_flowhash(struct ifnet *ifp)
{
	struct ifnet_flowhash_key fh __attribute__((aligned(8)));
	uint32_t flowhash = 0;

	if (ifnet_flowhash_seed == 0) {
		ifnet_flowhash_seed = RandomULong();
	}

	bzero(&fh, sizeof(fh));

	(void) snprintf(fh.ifk_name, sizeof(fh.ifk_name), "%s", ifp->if_name);
	fh.ifk_unit = ifp->if_unit;
	fh.ifk_flags = ifp->if_flags;
	fh.ifk_eflags = ifp->if_eflags;
	fh.ifk_capabilities = ifp->if_capabilities;
	fh.ifk_capenable = ifp->if_capenable;
	fh.ifk_output_sched_model = ifp->if_output_sched_model;
	fh.ifk_rand1 = RandomULong();
	fh.ifk_rand2 = RandomULong();

try_again:
	flowhash = net_flowhash(&fh, sizeof(fh), ifnet_flowhash_seed);
	if (flowhash == 0) {
		                                    
		ifnet_flowhash_seed = RandomULong();
		goto try_again;
	}

	return flowhash;
}

int
ifnet_set_netsignature(struct ifnet *ifp, uint8_t family, uint8_t len,
    uint16_t flags, uint8_t *data)
{
#pragma unused(flags)
	int error = 0;

	switch (family) {
	case AF_INET:
		if_inetdata_lock_exclusive(ifp);
		if (IN_IFEXTRA(ifp) != NULL) {
			if (len == 0) {
				                                  
				IN_IFEXTRA(ifp)->netsig_len = 0;
				bzero(IN_IFEXTRA(ifp)->netsig,
				    sizeof(IN_IFEXTRA(ifp)->netsig));
				if_inetdata_lock_done(ifp);
				break;
			} else if (len > sizeof(IN_IFEXTRA(ifp)->netsig)) {
				error = EINVAL;
				if_inetdata_lock_done(ifp);
				break;
			}
			IN_IFEXTRA(ifp)->netsig_len = len;
			bcopy(data, IN_IFEXTRA(ifp)->netsig, len);
		} else {
			error = ENOMEM;
		}
		if_inetdata_lock_done(ifp);
		break;

	case AF_INET6:
		if_inet6data_lock_exclusive(ifp);
		if (IN6_IFEXTRA(ifp) != NULL) {
			if (len == 0) {
				                                  
				IN6_IFEXTRA(ifp)->netsig_len = 0;
				bzero(IN6_IFEXTRA(ifp)->netsig,
				    sizeof(IN6_IFEXTRA(ifp)->netsig));
				if_inet6data_lock_done(ifp);
				break;
			} else if (len > sizeof(IN6_IFEXTRA(ifp)->netsig)) {
				error = EINVAL;
				if_inet6data_lock_done(ifp);
				break;
			}
			IN6_IFEXTRA(ifp)->netsig_len = len;
			bcopy(data, IN6_IFEXTRA(ifp)->netsig, len);
		} else {
			error = ENOMEM;
		}
		if_inet6data_lock_done(ifp);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

int
ifnet_get_netsignature(struct ifnet *ifp, uint8_t family, uint8_t *len,
    uint16_t *flags, uint8_t *data)
{
	int error = 0;

	if (ifp == NULL || len == NULL || data == NULL) {
		return EINVAL;
	}

	switch (family) {
	case AF_INET:
		if_inetdata_lock_shared(ifp);
		if (IN_IFEXTRA(ifp) != NULL) {
			if (*len == 0 || *len < IN_IFEXTRA(ifp)->netsig_len) {
				error = EINVAL;
				if_inetdata_lock_done(ifp);
				break;
			}
			if ((*len = (uint8_t)IN_IFEXTRA(ifp)->netsig_len) > 0) {
				bcopy(IN_IFEXTRA(ifp)->netsig, data, *len);
			} else {
				error = ENOENT;
			}
		} else {
			error = ENOMEM;
		}
		if_inetdata_lock_done(ifp);
		break;

	case AF_INET6:
		if_inet6data_lock_shared(ifp);
		if (IN6_IFEXTRA(ifp) != NULL) {
			if (*len == 0 || *len < IN6_IFEXTRA(ifp)->netsig_len) {
				error = EINVAL;
				if_inet6data_lock_done(ifp);
				break;
			}
			if ((*len = (uint8_t)IN6_IFEXTRA(ifp)->netsig_len) > 0) {
				bcopy(IN6_IFEXTRA(ifp)->netsig, data, *len);
			} else {
				error = ENOENT;
			}
		} else {
			error = ENOMEM;
		}
		if_inet6data_lock_done(ifp);
		break;

	default:
		error = EINVAL;
		break;
	}

	if (error == 0 && flags != NULL) {
		*flags = 0;
	}

	return error;
}

int
ifnet_set_nat64prefix(struct ifnet *ifp, struct ipv6_prefix *prefixes)
{
	int i, error = 0, one_set = 0;

	if_inet6data_lock_exclusive(ifp);

	if (IN6_IFEXTRA(ifp) == NULL) {
		error = ENOMEM;
		goto out;
	}

	for (i = 0; i < NAT64_MAX_NUM_PREFIXES; i++) {
		uint32_t prefix_len =
		    prefixes[i].prefix_len;
		struct in6_addr *prefix =
		    &prefixes[i].ipv6_prefix;

		if (prefix_len == 0) {
			clat_log0((LOG_DEBUG,
			    "NAT64 prefixes purged from Interface %s\n",
			    if_name(ifp)));
			                                  
			IN6_IFEXTRA(ifp)->nat64_prefixes[i].prefix_len = 0;
			bzero(&IN6_IFEXTRA(ifp)->nat64_prefixes[i].ipv6_prefix,
			    sizeof(struct in6_addr));

			continue;
		} else if (prefix_len != NAT64_PREFIX_LEN_32 &&
		    prefix_len != NAT64_PREFIX_LEN_40 &&
		    prefix_len != NAT64_PREFIX_LEN_48 &&
		    prefix_len != NAT64_PREFIX_LEN_56 &&
		    prefix_len != NAT64_PREFIX_LEN_64 &&
		    prefix_len != NAT64_PREFIX_LEN_96) {
			clat_log0((LOG_DEBUG,
			    "NAT64 prefixlen is incorrect %d\n", prefix_len));
			error = EINVAL;
			goto out;
		}

		if (IN6_IS_SCOPE_EMBED(prefix)) {
			clat_log0((LOG_DEBUG,
			    "NAT64 prefix has interface/link local scope.\n"));
			error = EINVAL;
			goto out;
		}

		IN6_IFEXTRA(ifp)->nat64_prefixes[i].prefix_len = prefix_len;
		bcopy(prefix, &IN6_IFEXTRA(ifp)->nat64_prefixes[i].ipv6_prefix,
		    sizeof(struct in6_addr));
		clat_log0((LOG_DEBUG,
		    "NAT64 prefix set to %s with prefixlen: %d\n",
		    ip6_sprintf(prefix), prefix_len));
		one_set = 1;
	}

out:
	if_inet6data_lock_done(ifp);

	if (error == 0 && one_set != 0) {
		necp_update_all_clients();
	}

	return error;
}

int
ifnet_get_nat64prefix(struct ifnet *ifp, struct ipv6_prefix *prefixes)
{
	int i, found_one = 0, error = 0;

	if (ifp == NULL) {
		return EINVAL;
	}

	if_inet6data_lock_shared(ifp);

	if (IN6_IFEXTRA(ifp) == NULL) {
		error = ENOMEM;
		goto out;
	}

	for (i = 0; i < NAT64_MAX_NUM_PREFIXES; i++) {
		if (IN6_IFEXTRA(ifp)->nat64_prefixes[i].prefix_len != 0) {
			found_one = 1;
		}
	}

	if (found_one == 0) {
		error = ENOENT;
		goto out;
	}

	if (prefixes) {
		bcopy(IN6_IFEXTRA(ifp)->nat64_prefixes, prefixes,
		    sizeof(IN6_IFEXTRA(ifp)->nat64_prefixes));
	}

out:
	if_inet6data_lock_done(ifp);

	return error;
}

static void
dlil_output_cksum_dbg(struct ifnet *ifp, struct mbuf *m, uint32_t hoff,
    protocol_family_t pf)
{
#pragma unused(ifp)
	uint32_t did_sw;

	if (!(hwcksum_dbg_mode & HWCKSUM_DBG_FINALIZE_FORCED) ||
	    (m->m_pkthdr.csum_flags & (CSUM_TSO_IPV4 | CSUM_TSO_IPV6))) {
		return;
	}

	switch (pf) {
	case PF_INET:
		did_sw = in_finalize_cksum(m, hoff, m->m_pkthdr.csum_flags);
		if (did_sw & CSUM_DELAY_IP) {
			hwcksum_dbg_finalized_hdr++;
		}
		if (did_sw & CSUM_DELAY_DATA) {
			hwcksum_dbg_finalized_data++;
		}
		break;
	case PF_INET6:
		  
                                                       
                                                     
                                                          
                                                       
                          
     
		did_sw = in6_finalize_cksum(m, hoff, -1, -1,
		    m->m_pkthdr.csum_flags);
		if (did_sw & CSUM_DELAY_IPV6_DATA) {
			hwcksum_dbg_finalized_data++;
		}
		break;
	default:
		return;
	}
}

static void
dlil_input_cksum_dbg(struct ifnet *ifp, struct mbuf *m, char *frame_header,
    protocol_family_t pf)
{
	uint16_t sum = 0;
	uint32_t hlen;

	if (frame_header == NULL ||
	    frame_header < (char *)mbuf_datastart(m) ||
	    frame_header > (char *)m->m_data) {
		DLIL_PRINTF("%s: frame header pointer 0x%llx out of range "
		    "[0x%llx,0x%llx] for mbuf 0x%llx\n", if_name(ifp),
		    (uint64_t)VM_KERNEL_ADDRPERM(frame_header),
		    (uint64_t)VM_KERNEL_ADDRPERM(mbuf_datastart(m)),
		    (uint64_t)VM_KERNEL_ADDRPERM(m->m_data),
		    (uint64_t)VM_KERNEL_ADDRPERM(m));
		return;
	}
	hlen = (uint32_t)(m->m_data - frame_header);

	switch (pf) {
	case PF_INET:
	case PF_INET6:
		break;
	default:
		return;
	}

	  
                                                            
                                                                 
                                                                 
    
	if (hwcksum_dbg_mode & HWCKSUM_DBG_PARTIAL_FORCED) {
		uint32_t foff = hwcksum_dbg_partial_rxoff_forced;

		if (foff > (uint32_t)m->m_pkthdr.len) {
			return;
		}

		m->m_pkthdr.csum_flags &= ~CSUM_RX_FLAGS;

		                                                          
		sum = m_sum16(m, foff, (m->m_pkthdr.len - foff));

		m->m_pkthdr.csum_flags |= (CSUM_DATA_VALID | CSUM_PARTIAL);
		m->m_pkthdr.csum_rx_val = sum;
		m->m_pkthdr.csum_rx_start = (uint16_t)(foff + hlen);

		hwcksum_dbg_partial_forced++;
		hwcksum_dbg_partial_forced_bytes += m->m_pkthdr.len;
	}

	  
                                                           
                                                        
                                      
    
	if ((m->m_pkthdr.csum_flags &
	    (CSUM_DATA_VALID | CSUM_PARTIAL | CSUM_PSEUDO_HDR)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		uint32_t rxoff;

		                                                
		rxoff = m->m_pkthdr.csum_rx_start;
		if (hlen > rxoff) {
			hwcksum_dbg_bad_rxoff++;
			if (dlil_verbose) {
				DLIL_PRINTF("%s: partial cksum start offset %d "
				    "is less than frame header length %d for "
				    "mbuf 0x%llx\n", if_name(ifp), rxoff, hlen,
				    (uint64_t)VM_KERNEL_ADDRPERM(m));
			}
			return;
		}
		rxoff -= hlen;

		if (!(hwcksum_dbg_mode & HWCKSUM_DBG_PARTIAL_FORCED)) {
			  
                                                     
                                                  
                                              
      
			sum = m_sum16(m, rxoff, (m->m_pkthdr.len - rxoff));

			                                 
			if (sum != m->m_pkthdr.csum_rx_val) {
				hwcksum_dbg_bad_cksum++;
				if (dlil_verbose) {
					DLIL_PRINTF("%s: bad partial cksum value "
					    "0x%x (expected 0x%x) for mbuf "
					    "0x%llx [rx_start %d]\n",
					    if_name(ifp),
					    m->m_pkthdr.csum_rx_val, sum,
					    (uint64_t)VM_KERNEL_ADDRPERM(m),
					    m->m_pkthdr.csum_rx_start);
				}
				return;
			}
		}
		hwcksum_dbg_verified++;

		  
                                                          
                                                           
                         
     
		if (hwcksum_dbg_mode & HWCKSUM_DBG_PARTIAL_RXOFF_ADJ) {
			uint32_t aoff = hwcksum_dbg_partial_rxoff_adj;

			if (aoff == rxoff || aoff > (uint32_t)m->m_pkthdr.len) {
				return;
			}

			sum = m_adj_sum16(m, rxoff, aoff,
			    m_pktlen(m) - aoff, sum);

			m->m_pkthdr.csum_rx_val = sum;
			m->m_pkthdr.csum_rx_start = (uint16_t)(aoff + hlen);

			hwcksum_dbg_adjusted++;
		}
	}
}

static int
sysctl_hwcksum_dbg_mode SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	u_int32_t i;
	int err;

	i = hwcksum_dbg_mode;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (hwcksum_dbg == 0) {
		return ENODEV;
	}

	if ((i & ~HWCKSUM_DBG_MASK) != 0) {
		return EINVAL;
	}

	hwcksum_dbg_mode = (i & HWCKSUM_DBG_MASK);

	return err;
}

static int
sysctl_hwcksum_dbg_partial_rxoff_forced SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	u_int32_t i;
	int err;

	i = hwcksum_dbg_partial_rxoff_forced;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (!(hwcksum_dbg_mode & HWCKSUM_DBG_PARTIAL_FORCED)) {
		return ENODEV;
	}

	hwcksum_dbg_partial_rxoff_forced = i;

	return err;
}

static int
sysctl_hwcksum_dbg_partial_rxoff_adj SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	u_int32_t i;
	int err;

	i = hwcksum_dbg_partial_rxoff_adj;

	err = sysctl_handle_int(oidp, &i, 0, req);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (!(hwcksum_dbg_mode & HWCKSUM_DBG_PARTIAL_RXOFF_ADJ)) {
		return ENODEV;
	}

	hwcksum_dbg_partial_rxoff_adj = i;

	return err;
}

static int
sysctl_tx_chain_len_stats SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int err;

	if (req->oldptr == USER_ADDR_NULL) {
	}
	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}
	err = SYSCTL_OUT(req, &tx_chain_len_stats,
	    sizeof(struct chain_len_stats));

	return err;
}


#if DEBUG || DEVELOPMENT
                                 
static uint8_t sumdata[] = {
	0x1f, 0x8b, 0x08, 0x08, 0x4c, 0xe5, 0x9a, 0x4f, 0x00, 0x03,
	0x5f, 0x00, 0x5d, 0x91, 0x41, 0x4e, 0xc4, 0x30, 0x0c, 0x45,
	0xf7, 0x9c, 0xc2, 0x07, 0x18, 0xf5, 0x0e, 0xb0, 0xe2, 0x00,
	0x48, 0x88, 0xa5, 0xdb, 0xba, 0x49, 0x34, 0x69, 0xdc, 0x71,
	0x92, 0xa9, 0xc2, 0x8a, 0x6b, 0x70, 0x3d, 0x4e, 0x82, 0x93,
	0xb4, 0x08, 0xd8, 0xc5, 0xb1, 0xfd, 0xff, 0xb3, 0xfd, 0x4c,
	0x42, 0x5f, 0x1f, 0x9f, 0x11, 0x12, 0x43, 0xb2, 0x04, 0x93,
	0xe0, 0x7b, 0x01, 0x0e, 0x14, 0x07, 0x78, 0xd1, 0x78, 0x75,
	0x71, 0x71, 0xe9, 0x08, 0x84, 0x46, 0xf2, 0xc7, 0x3b, 0x09,
	0xe7, 0xd1, 0xd3, 0x8a, 0x57, 0x92, 0x33, 0xcd, 0x39, 0xcc,
	0xb0, 0x91, 0x89, 0xe0, 0x42, 0x53, 0x8b, 0xb7, 0x8c, 0x42,
	0x60, 0xd9, 0x9f, 0x7a, 0x55, 0x19, 0x76, 0xcb, 0x10, 0x49,
	0x35, 0xac, 0x0b, 0x5a, 0x3c, 0xbb, 0x65, 0x51, 0x8c, 0x90,
	0x7c, 0x69, 0x45, 0x45, 0x81, 0xb4, 0x2b, 0x70, 0x82, 0x85,
	0x55, 0x91, 0x17, 0x90, 0xdc, 0x14, 0x1e, 0x35, 0x52, 0xdd,
	0x02, 0x16, 0xef, 0xb5, 0x40, 0x89, 0xe2, 0x46, 0x53, 0xad,
	0x93, 0x6e, 0x98, 0x30, 0xe5, 0x08, 0xb7, 0xcc, 0x03, 0xbc,
	0x71, 0x86, 0x09, 0x43, 0x0d, 0x52, 0xf5, 0xa2, 0xf5, 0xa2,
	0x56, 0x11, 0x8d, 0xa8, 0xf5, 0xee, 0x92, 0x3d, 0xfe, 0x8c,
	0x67, 0x71, 0x8b, 0x0e, 0x2d, 0x70, 0x77, 0xbe, 0xbe, 0xea,
	0xbf, 0x9a, 0x8d, 0x9c, 0x53, 0x53, 0xe5, 0xe0, 0x4b, 0x87,
	0x85, 0xd2, 0x45, 0x95, 0x30, 0xc1, 0xcc, 0xe0, 0x74, 0x54,
	0x13, 0x58, 0xe8, 0xe8, 0x79, 0xa2, 0x09, 0x73, 0xa4, 0x0e,
	0x39, 0x59, 0x0c, 0xe6, 0x9c, 0xb2, 0x4f, 0x06, 0x5b, 0x8e,
	0xcd, 0x17, 0x6c, 0x5e, 0x95, 0x4d, 0x70, 0xa2, 0x0a, 0xbf,
	0xa3, 0xcc, 0x03, 0xbc, 0x5a, 0xe7, 0x75, 0x06, 0x5e, 0x75,
	0xef, 0x58, 0x8e, 0x15, 0xd1, 0x0a, 0x18, 0xff, 0xdd, 0xe6,
	0x02, 0x3b, 0xb5, 0xb4, 0xa1, 0xe0, 0x72, 0xfc, 0xe3, 0xab,
	0x07, 0xe0, 0x4d, 0x65, 0xea, 0x92, 0xeb, 0xf2, 0x7b, 0x17,
	0x05, 0xce, 0xc6, 0xf6, 0x2b, 0xbb, 0x70, 0x3d, 0x00, 0x95,
	0xe0, 0x07, 0x52, 0x3b, 0x58, 0xfc, 0x7c, 0x69, 0x4d, 0xe9,
	0xf7, 0xa9, 0x66, 0x1e, 0x1e, 0xbe, 0x01, 0x69, 0x98, 0xfe,
	0xc8, 0x28, 0x02, 0x00, 0x00
};

                                                                                
static struct {
	boolean_t       init;
	uint16_t        len;
	uint16_t        sumr;                  
	uint16_t        sumrp;                              
} sumtbl[] = {
	{ FALSE, 0, 0, 0x0000 },
	{ FALSE, 1, 0, 0x001f },
	{ FALSE, 2, 0, 0x8b1f },
	{ FALSE, 3, 0, 0x8b27 },
	{ FALSE, 7, 0, 0x790e },
	{ FALSE, 11, 0, 0xcb6d },
	{ FALSE, 20, 0, 0x20dd },
	{ FALSE, 27, 0, 0xbabd },
	{ FALSE, 32, 0, 0xf3e8 },
	{ FALSE, 37, 0, 0x197d },
	{ FALSE, 43, 0, 0x9eae },
	{ FALSE, 64, 0, 0x4678 },
	{ FALSE, 127, 0, 0x9399 },
	{ FALSE, 256, 0, 0xd147 },
	{ FALSE, 325, 0, 0x0358 },
};
#define SUMTBL_MAX      ((int)sizeof (sumtbl) / (int)sizeof (sumtbl[0]))

static void
dlil_verify_sum16(void)
{
	struct mbuf *m;
	uint8_t *buf;
	int n;

	                                                                       
	_CASSERT((sizeof(sumdata) + (sizeof(uint64_t) * 2)) <= MCLBYTES);

	kprintf("DLIL: running SUM16 self-tests ... ");

	m = m_getcl(M_WAITOK, MT_DATA, M_PKTHDR);
	m_align(m, sizeof(sumdata) + (sizeof(uint64_t) * 2));

	buf = mtod(m, uint8_t *);                                 

	for (n = 0; n < SUMTBL_MAX; n++) {
		uint16_t len = sumtbl[n].len;
		int i;

		                                        
		for (i = 0; i < (int)sizeof(uint64_t); i++) {
			uint16_t sum, sumr;
			uint8_t *c;

			                                 
			VERIFY(len <= sizeof(sumdata));
			c = buf + i;
			bcopy(sumdata, c, len);

			                                              
			m->m_data = (caddr_t)c;
			m->m_len = len;
			sum = m_sum16(m, 0, len);

			if (!sumtbl[n].init) {
				sumr = (uint16_t)in_cksum_mbuf_ref(m, len, 0, 0);
				sumtbl[n].sumr = sumr;
				sumtbl[n].init = TRUE;
			} else {
				sumr = sumtbl[n].sumr;
			}

			                                            
			if (sumr != sumtbl[n].sumrp) {
				panic_plain("\n%s: broken in_cksum_mbuf_ref() "
				    "for len=%d align=%d sum=0x%04x "
				    "[expected=0x%04x]\n", __func__,
				    len, i, sum, sumr);
				                
			} else if (sum != sumr) {
				panic_plain("\n%s: broken m_sum16() for len=%d "
				    "align=%d sum=0x%04x [expected=0x%04x]\n",
				    __func__, len, i, sum, sumr);
				                
			}

			                                                   
			m->m_data = (caddr_t)buf;
			m->m_len = i + len;
			sum = m_sum16(m, i, len);

			                                            
			if (sum != sumr) {
				panic_plain("\n%s: broken m_sum16() for len=%d "
				    "offset=%d sum=0x%04x [expected=0x%04x]\n",
				    __func__, len, i, sum, sumr);
				                
			}
#if INET
			                                                     
			sum = b_sum16(c, len);

			                                            
			if (sum != sumr) {
				panic_plain("\n%s: broken b_sum16() for len=%d "
				    "align=%d sum=0x%04x [expected=0x%04x]\n",
				    __func__, len, i, sum, sumr);
				                
			}
#endif           
		}
	}
	m_freem(m);

	kprintf("PASSED\n");
}
#endif                           

#define CASE_STRINGIFY(x) case x: return #x

__private_extern__ const char *
dlil_kev_dl_code_str(u_int32_t event_code)
{
	switch (event_code) {
		CASE_STRINGIFY(KEV_DL_SIFFLAGS);
		CASE_STRINGIFY(KEV_DL_SIFMETRICS);
		CASE_STRINGIFY(KEV_DL_SIFMTU);
		CASE_STRINGIFY(KEV_DL_SIFPHYS);
		CASE_STRINGIFY(KEV_DL_SIFMEDIA);
		CASE_STRINGIFY(KEV_DL_SIFGENERIC);
		CASE_STRINGIFY(KEV_DL_ADDMULTI);
		CASE_STRINGIFY(KEV_DL_DELMULTI);
		CASE_STRINGIFY(KEV_DL_IF_ATTACHED);
		CASE_STRINGIFY(KEV_DL_IF_DETACHING);
		CASE_STRINGIFY(KEV_DL_IF_DETACHED);
		CASE_STRINGIFY(KEV_DL_LINK_OFF);
		CASE_STRINGIFY(KEV_DL_LINK_ON);
		CASE_STRINGIFY(KEV_DL_PROTO_ATTACHED);
		CASE_STRINGIFY(KEV_DL_PROTO_DETACHED);
		CASE_STRINGIFY(KEV_DL_LINK_ADDRESS_CHANGED);
		CASE_STRINGIFY(KEV_DL_WAKEFLAGS_CHANGED);
		CASE_STRINGIFY(KEV_DL_IF_IDLE_ROUTE_REFCNT);
		CASE_STRINGIFY(KEV_DL_IFCAP_CHANGED);
		CASE_STRINGIFY(KEV_DL_LINK_QUALITY_METRIC_CHANGED);
		CASE_STRINGIFY(KEV_DL_NODE_PRESENCE);
		CASE_STRINGIFY(KEV_DL_NODE_ABSENCE);
		CASE_STRINGIFY(KEV_DL_MASTER_ELECTED);
		CASE_STRINGIFY(KEV_DL_ISSUES);
		CASE_STRINGIFY(KEV_DL_IFDELEGATE_CHANGED);
	default:
		break;
	}
	return "";
}

static void
dlil_dt_tcall_fn(thread_call_param_t arg0, thread_call_param_t arg1)
{
#pragma unused(arg1)
	struct ifnet *ifp = arg0;

	if (ifnet_is_attached(ifp, 1)) {
		nstat_ifnet_threshold_reached(ifp->if_index);
		ifnet_decr_iorefcnt(ifp);
	}
}

void
ifnet_notify_data_threshold(struct ifnet *ifp)
{
	uint64_t bytes = (ifp->if_ibytes + ifp->if_obytes);
	uint64_t oldbytes = ifp->if_dt_bytes;

	ASSERT(ifp->if_dt_tcall != NULL);

	  
                                                            
                                                           
    
	if (threshold_notify && (bytes - oldbytes) > ifp->if_data_threshold &&
	    OSCompareAndSwap64(oldbytes, bytes, &ifp->if_dt_bytes) &&
	    !thread_call_isactive(ifp->if_dt_tcall)) {
		uint64_t tival = (threshold_interval * NSEC_PER_SEC);
		uint64_t now = mach_absolute_time(), deadline = now;
		uint64_t ival;

		if (tival != 0) {
			nanoseconds_to_absolutetime(tival, &ival);
			clock_deadline_for_periodic_event(ival, now, &deadline);
			(void) thread_call_enter_delayed(ifp->if_dt_tcall,
			    deadline);
		} else {
			(void) thread_call_enter(ifp->if_dt_tcall);
		}
	}
}

#if (DEVELOPMENT || DEBUG)
  
                                                            
                                       
                                  
                                  
                                  
                                            
   
static int
sysctl_get_kao_frames SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp)
	int *name = (int *)arg1;
	u_int namelen = arg2;
	int idx;
	ifnet_t ifp = NULL;
	u_int32_t frames_array_count;
	size_t frame_data_offset;
	u_int32_t used_frames_count;
	struct ifnet_keepalive_offload_frame *frames_array = NULL;
	int error = 0;
	u_int32_t i;

	  
                                                     
    
	error = proc_suser(current_proc());
	if (error != 0) {
		goto done;
	}
	  
                                 
    
	if (req->newptr != USER_ADDR_NULL) {
		error = EPERM;
		goto done;
	}
	if (namelen != 3) {
		error = EINVAL;
		goto done;
	}
	if (req->oldptr == USER_ADDR_NULL) {
		error = EINVAL;
		goto done;
	}
	if (req->oldlen == 0) {
		error = EINVAL;
		goto done;
	}
	idx = name[0];
	frames_array_count = name[1];
	frame_data_offset = name[2];

	                                                 
	if (frames_array_count * sizeof(struct ifnet_keepalive_offload_frame) >
	    req->oldlen) {
		error = ENOMEM;
		goto done;
	}

	ifnet_head_lock_shared();
	if (!IF_INDEX_IN_RANGE(idx)) {
		ifnet_head_done();
		error = ENOENT;
		goto done;
	}
	ifp = ifindex2ifnet[idx];
	ifnet_head_done();

	frames_array = _MALLOC(frames_array_count *
	    sizeof(struct ifnet_keepalive_offload_frame), M_TEMP, M_WAITOK);
	if (frames_array == NULL) {
		error = ENOMEM;
		goto done;
	}

	error = ifnet_get_keepalive_offload_frames(ifp, frames_array,
	    frames_array_count, frame_data_offset, &used_frames_count);
	if (error != 0) {
		DLIL_PRINTF("%s: ifnet_get_keepalive_offload_frames error %d\n",
		    __func__, error);
		goto done;
	}

	for (i = 0; i < used_frames_count; i++) {
		error = SYSCTL_OUT(req, frames_array + i,
		    sizeof(struct ifnet_keepalive_offload_frame));
		if (error != 0) {
			goto done;
		}
	}
done:
	if (frames_array != NULL) {
		_FREE(frames_array, M_TEMP);
	}
	return error;
}
#endif                           

void
ifnet_update_stats_per_flow(struct ifnet_stats_per_flow *ifs,
    struct ifnet *ifp)
{
	tcp_update_stats_per_flow(ifs, ifp);
}

static inline u_int32_t
_set_flags(u_int32_t *flags_p, u_int32_t set_flags)
{
	return (u_int32_t)OSBitOrAtomic(set_flags, flags_p);
}

static inline void
_clear_flags(u_int32_t *flags_p, u_int32_t clear_flags)
{
	OSBitAndAtomic(~clear_flags, flags_p);
}

__private_extern__ u_int32_t
if_set_eflags(ifnet_t interface, u_int32_t set_flags)
{
	return _set_flags(&interface->if_eflags, set_flags);
}

__private_extern__ void
if_clear_eflags(ifnet_t interface, u_int32_t clear_flags)
{
	_clear_flags(&interface->if_eflags, clear_flags);
}

__private_extern__ u_int32_t
if_set_xflags(ifnet_t interface, u_int32_t set_flags)
{
	return _set_flags(&interface->if_xflags, set_flags);
}

__private_extern__ void
if_clear_xflags(ifnet_t interface, u_int32_t clear_flags)
{
	_clear_flags(&interface->if_xflags, clear_flags);
}
