/*
 * Copyright (c) 2000-2017 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)route.h	8.3 (Berkeley) 4/19/94
 * $FreeBSD: src/sys/net/route.h,v 1.36.2.1 2000/08/16 06:14:23 jayanth Exp $
 */

#ifndef _NET_ROUTE_H_
#define _NET_ROUTE_H_
#include <sys/appleapiopts.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

  
                                                               
                                                                     
   
struct rt_metrics {
	u_int32_t       rmx_locks;                                            
	u_int32_t       rmx_mtu;                               
	u_int32_t       rmx_hopcount;                          
	int32_t         rmx_expire;                                            
	u_int32_t       rmx_recvpipe;                                        
	u_int32_t       rmx_sendpipe;                                         
	u_int32_t       rmx_ssthresh;                                      
	u_int32_t       rmx_rtt;                                       
	u_int32_t       rmx_rttvar;                                 
	u_int32_t       rmx_pksent;                                        
	u_int32_t       rmx_state;                       
	u_int32_t       rmx_filler[3];                                             
};

  
                                                     
   
#define RTM_RTTUNIT     1000000                                              

#ifdef PRIVATE
struct route_old {
	void            *ro_rt;
	uint32_t        ro_flags;
	struct sockaddr ro_dst;
};
#endif              

#ifdef BSD_KERNEL_PRIVATE
#include <kern/locks.h>
#include <net/radix.h>
#include <net/if_llatbl.h>
#include <sys/eventhandler.h>
#include <net/if_dl.h>

extern boolean_t trigger_v6_defrtr_select;
  
                                  
  
                                                              
                                                                   
   

                          
struct ifnet_llreach_info;
struct rt_reach_info;

  
                     
  
                                                            
                                                         
                                       
   
struct route {
	  
                                                                
                                                              
                          
    
	struct rtentry  *ro_rt;
	struct llentry  *ro_lle;

	struct ifaddr   *ro_srcia;
	uint32_t        ro_flags;                                    
	struct sockaddr ro_dst;
};

#define ROF_SRCIF_SELECTED      0x0001                                     
#if 0
                                                           
#define        ROF_NORTREF             0x0002                                       
#define        ROF_L2_ME               0x0004                                  
#define        ROF_MAY_LOOP            0x0008                                 
#define        ROF_HAS_HEADER          0x0010                                              
#define        ROF_REJECT              0x0020                             
#define        ROF_BLACKHOLE           0x0040                                
#define        ROF_HAS_GW              0x0080                           
#endif
#define ROF_LLE_CACHE   0x0100                         

#define ROUTE_UNUSABLE(_ro)                                             \
	((_ro)->ro_rt == NULL ||                                        \
	((_ro)->ro_rt->rt_flags & (RTF_UP|RTF_CONDEMNED)) != RTF_UP ||  \
	RT_GENID_OUTOFSYNC((_ro)->ro_rt))

#define _ROUTE_RELEASE_COMMON(_ro, _rnh_locked) do {                    \
	if ((_ro)->ro_rt != NULL) {                                     \
	        RT_LOCK_ASSERT_NOTHELD((_ro)->ro_rt);                   \
	        if (_rnh_locked)                                        \
	                rtfree_locked((_ro)->ro_rt);                    \
	        else                                                    \
	                rtfree((_ro)->ro_rt);                           \
	        (_ro)->ro_rt = NULL;                                    \
	}                                                               \
	if ((_ro)->ro_srcia != NULL) {                                  \
	        IFA_REMREF((_ro)->ro_srcia);                            \
	        (_ro)->ro_srcia = NULL;                                 \
	        (_ro)->ro_flags &= ~ROF_SRCIF_SELECTED;                 \
	}                                                               \
	if ((_ro)->ro_lle != NULL) {                                    \
	        LLE_REMREF((_ro)->ro_lle);                              \
	        (_ro)->ro_lle = NULL;                                   \
	        (_ro)->ro_flags &= ~ROF_LLE_CACHE;                      \
	}                                                               \
} while (0)

#define ROUTE_RELEASE_LOCKED(_ro)       _ROUTE_RELEASE_COMMON(_ro, TRUE)
#define ROUTE_RELEASE(_ro)              _ROUTE_RELEASE_COMMON(_ro, FALSE)

  
                                                                 
                                                               
                                                              
                                                              
                                                                      
                                                
   

#define NRTT_HIST       10
  
                                  
   
struct rtentry {
	struct  radix_node rt_nodes[2];                                  
#define rt_key(r)       (SA((r)->rt_nodes->rn_key))
#define rt_mask(r)      (SA((r)->rt_nodes->rn_mask))
	  
                                                  
    
	decl_lck_mtx_data(, rt_lock);                               
	uint32_t rt_refcnt;                                    
	uint32_t rt_flags;                                      
	uint32_t rt_genid;                                       
	struct sockaddr *rt_gateway;               
	struct ifnet *rt_ifp;                                             
	struct ifaddr *rt_ifa;                                                 
	struct sockaddr *rt_genmask;                                         
	void *rt_llinfo;                                                      
	void (*rt_llinfo_get_ri)                                             
	(struct rtentry *, struct rt_reach_info *);
	void (*rt_llinfo_get_iflri)                                          
	(struct rtentry *, struct ifnet_llreach_info *);
	void (*rt_llinfo_purge)(struct rtentry *);                      
	void (*rt_llinfo_free)(void *);                                    
	void (*rt_llinfo_refresh) (struct rtentry *);                              
	struct rt_metrics rt_rmx;                                             
#define rt_use rt_rmx.rmx_pksent
	struct rtentry *rt_gwroute;                                             
	struct rtentry *rt_parent;                                        
	struct nstat_counts *rt_stats;                   
	void (*rt_if_ref_fn)(struct ifnet *, int);                         

	uint32_t *rt_tree_genid;                                         
	uint64_t rt_expire;                                                    
	uint64_t base_calendartime;                                            
	uint64_t base_uptime;                                           
	u_int32_t rtt_hist[NRTT_HIST];                                             
	u_int32_t rtt_min;                                                     
	u_int32_t rtt_expire_ts;                                          
	u_int8_t rtt_index;                                         
	                                           
	struct eventhandler_lists_ctxt rt_evhdlr_ctxt;
};

enum {
	ROUTE_STATUS_UPDATE = 1,
	ROUTE_ENTRY_REFRESH,
	ROUTE_ENTRY_DELETED,
	ROUTE_LLENTRY_RESOLVED,
	ROUTE_LLENTRY_UNREACH,
	ROUTE_LLENTRY_CHANGED,
	ROUTE_LLENTRY_STALE,
	ROUTE_LLENTRY_TIMEDOUT,
	ROUTE_LLENTRY_DELETED,
	ROUTE_LLENTRY_EXPIRED,
	ROUTE_LLENTRY_PROBED,
	ROUTE_EVHDLR_DEREGISTER,
};

extern const char * route_event2str(int route_event);

typedef void (*route_event_fn) (struct eventhandler_entry_arg,
    struct sockaddr *, int, struct sockaddr *, int);
EVENTHANDLER_DECLARE(route_event, route_event_fn);

  
                                                           
   
#define RT_GENID_SYNC(_rt) do {                                         \
	if ((_rt)->rt_tree_genid != NULL)                               \
	        (_rt)->rt_genid = *(_rt)->rt_tree_genid;                \
} while (0)

  
                                                                     
   
#define RT_GENID_OUTOFSYNC(_rt)                                         \
	((_rt)->rt_tree_genid != NULL &&                                \
	*(_rt)->rt_tree_genid != (_rt)->rt_genid)

enum {
	ROUTE_OP_READ,
	ROUTE_OP_WRITE,
};

extern int route_op_entitlement_check(struct socket *, kauth_cred_t, int, boolean_t);
#endif                         

#define RTF_UP          0x1                               
#define RTF_GATEWAY     0x2                                           
#define RTF_HOST        0x4                                             
#define RTF_REJECT      0x8                                          
#define RTF_DYNAMIC     0x10                                                   
#define RTF_MODIFIED    0x20                                                    
#define RTF_DONE        0x40                                   
#define RTF_DELCLONE    0x80                                     
#define RTF_CLONING     0x100                                           
#define RTF_XRESOLVE    0x200                                              
#define RTF_LLINFO      0x400                                                   
                                                     
#define RTF_LLDATA      0x400                                                   
#define RTF_STATIC      0x800                               
#define RTF_BLACKHOLE   0x1000                                                  
#define RTF_NOIFREF     0x2000                                          
#define RTF_PROTO2      0x4000                                              
#define RTF_PROTO1      0x8000                                              

#define RTF_PRCLONING   0x10000                                        
#define RTF_WASCLONED   0x20000                                              
#define RTF_PROTO3      0x40000                                             
                                                            
#define RTF_PINNED      0x100000                        
#define RTF_LOCAL       0x200000                                              
#define RTF_BROADCAST   0x400000                                              
#define RTF_MULTICAST   0x800000                                              
#define RTF_IFSCOPE     0x1000000                                      
#define RTF_CONDEMNED   0x2000000                                          
#define RTF_IFREF       0x4000000                                           
#define RTF_PROXY       0x8000000                                         
#define RTF_ROUTER      0x10000000                            
#define RTF_DEAD        0x20000000                                      
#define RTF_GLOBAL      0x40000000                                                       
                                                                   

#define RTPRF_OURS      RTF_PROTO3                                   
#define RTF_BITS \
	"\020\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE" \
	"\10DELCLONE\11CLONING\12XRESOLVE\13LLINFO\14STATIC\15BLACKHOLE" \
	"\16NOIFREF\17PROTO2\20PROTO1\21PRCLONING\22WASCLONED\23PROTO3" \
	"\25PINNED\26LOCAL\27BROADCAST\30MULTICAST\31IFSCOPE\32CONDEMNED" \
	"\33IFREF\34PROXY\35ROUTER\37GLOBAL"

#define IS_DIRECT_HOSTROUTE(rt) \
	(((rt)->rt_flags & (RTF_HOST | RTF_GATEWAY)) == RTF_HOST)
  
                      
   
struct  rtstat {
	short   rts_badredirect;                                  
	short   rts_dynamic;                                             
	short   rts_newgateway;                                           
	short   rts_unreach;                                      
	short   rts_wildcard;                                                
	short   rts_badrtgwroute;                                           
};

  
                                   
   
struct rt_msghdr {
	u_short rtm_msglen;                                               
	u_char  rtm_version;                                     
	u_char  rtm_type;                         
	u_short rtm_index;                                    
	int     rtm_flags;                                                  
	int     rtm_addrs;                                                
	pid_t   rtm_pid;                             
	int     rtm_seq;                                           
	int     rtm_errno;                      
	int     rtm_use;                          
	u_int32_t rtm_inits;                                           
	struct rt_metrics rtm_rmx;                         
};

struct rt_msghdr2 {
	u_short rtm_msglen;                                               
	u_char  rtm_version;                                     
	u_char  rtm_type;                         
	u_short rtm_index;                                    
	int     rtm_flags;                                                  
	int     rtm_addrs;                                                
	int32_t rtm_refcnt;                          
	int     rtm_parentflags;                                
	int     rtm_reserved;                                
	int     rtm_use;                          
	u_int32_t rtm_inits;                                           
	struct rt_metrics rtm_rmx;                         
};

#ifdef PRIVATE
struct kev_netevent_apnfallbk_data {
	pid_t           epid;                              
	uuid_t          euuid;                              
};

  
                           
   
struct rt_reach_info {
	u_int32_t       ri_refcnt;                           
	u_int32_t       ri_probes;                             
	u_int64_t       ri_snd_expire;                                     
	u_int64_t       ri_rcv_expire;                                     
	int32_t         ri_rssi;                                      
	int32_t         ri_lqm;                                  
	int32_t         ri_npm;                                    
};

  
                                             
   
struct rt_msghdr_ext {
	u_short rtm_msglen;                                               
	u_char  rtm_version;                                     
	u_char  rtm_type;                         
	u_int32_t rtm_index;                                  
	u_int32_t rtm_flags;                                                
	u_int32_t rtm_reserved;                     
	u_int32_t rtm_addrs;                                              
	pid_t   rtm_pid;                             
	int     rtm_seq;                                           
	int     rtm_errno;                      
	u_int32_t rtm_use;                        
	u_int32_t rtm_inits;                                           
	struct rt_metrics rtm_rmx;                              
	struct rt_reach_info rtm_ri;                                 
};
#endif              

#define RTM_VERSION     5                                                  

  
                 
   
#define RTM_ADD         0x1                    
#define RTM_DELETE      0x2                       
#define RTM_CHANGE      0x3                                  
#define RTM_GET         0x4                         
#define RTM_LOSING      0x5                                                
                                                 
#define RTM_REDIRECT    0x6                                      
#define RTM_MISS        0x7                                        
#define RTM_LOCK        0x8                                
#define RTM_OLDADD      0x9                              
#define RTM_OLDDEL      0xa                              
#define RTM_RESOLVE     0xb                                        
#define RTM_NEWADDR     0xc                                       
#define RTM_DELADDR     0xd                                           
#define RTM_IFINFO      0xe                                   
#define RTM_NEWMADDR    0xf                                                   
#define RTM_DELMADDR    0x10                                              
#ifdef PRIVATE
#define RTM_GET_SILENT  0x11
#endif              
#define RTM_IFINFO2     0x12         
#define RTM_NEWMADDR2   0x13         
#define RTM_GET2        0x14         
#ifdef PRIVATE
#define RTM_GET_EXT     0x15
#endif              

  
                                              
   
#define RTV_MTU         0x1                            
#define RTV_HOPCOUNT    0x2                                 
#define RTV_EXPIRE      0x4                               
#define RTV_RPIPE       0x8                                 
#define RTV_SPIPE       0x10                                
#define RTV_SSTHRESH    0x20                                
#define RTV_RTT         0x40                           
#define RTV_RTTVAR      0x80                              
#ifdef PRIVATE
#define RTV_REFRESH_HOST        0x100                                            
#endif

  
                                
   
#define RTA_DST         0x1                                       
#define RTA_GATEWAY     0x2                                   
#define RTA_NETMASK     0x4                                   
#define RTA_GENMASK     0x8                                        
#define RTA_IFP         0x10                                         
#define RTA_IFA         0x20                                         
#define RTA_AUTHOR      0x40                                         
#define RTA_BRD         0x80                                                 

  
                                                                    
   
#define RTAX_DST        0                                         
#define RTAX_GATEWAY    1                                     
#define RTAX_NETMASK    2                                     
#define RTAX_GENMASK    3                                          
#define RTAX_IFP        4                                            
#define RTAX_IFA        5                                            
#define RTAX_AUTHOR     6                                            
#define RTAX_BRD        7                                                    
#define RTAX_MAX        8                                      

struct rt_addrinfo {
	int     rti_addrs;
	struct  sockaddr *rti_info[RTAX_MAX];
};

#ifdef PRIVATE
  
                                                                       
   
#define IFSCOPE_NONE    0
#endif              

#ifdef BSD_KERNEL_PRIVATE
  
                                                                  
   
#define CTRACE_STACK_SIZE       8                                         
#define CTRACE_HIST_SIZE        4                                        
typedef struct ctrace {
	void    *th;                                            
	void    *pc[CTRACE_STACK_SIZE];                             
} ctrace_t;

extern void ctrace_record(ctrace_t *);

#define RT_LOCK_ASSERT_HELD(_rt)                                        \
	LCK_MTX_ASSERT(&(_rt)->rt_lock, LCK_MTX_ASSERT_OWNED)

#define RT_LOCK_ASSERT_NOTHELD(_rt)                                     \
	LCK_MTX_ASSERT(&(_rt)->rt_lock, LCK_MTX_ASSERT_NOTOWNED)

#define RT_LOCK(_rt) do {                                               \
	rt_lock(_rt, FALSE);                                            \
} while (0)

#define RT_LOCK_SPIN(_rt) do {                                          \
	rt_lock(_rt, TRUE);                                             \
} while (0)

#define RT_CONVERT_LOCK(_rt) do {                                       \
	RT_LOCK_ASSERT_HELD(_rt);                                       \
	lck_mtx_convert_spin(&(_rt)->rt_lock);                          \
} while (0)

#define RT_UNLOCK(_rt) do {                                             \
	rt_unlock(_rt);                                                 \
} while (0)

#define RT_ADDREF_LOCKED(_rt) do {                                      \
	rtref(_rt);                                                     \
} while (0)

  
                                                             
                                                             
   
#define RT_ADDREF(_rt) do {                                             \
	RT_LOCK_SPIN(_rt);                                              \
	RT_ADDREF_LOCKED(_rt);                                          \
	RT_UNLOCK(_rt);                                                 \
} while (0)

#define RT_REMREF_LOCKED(_rt) do {                                      \
	(void) rtunref(_rt);                                            \
} while (0)

  
                                                             
                                                             
   
#define RT_REMREF(_rt) do {                                             \
	RT_LOCK_SPIN(_rt);                                              \
	RT_REMREF_LOCKED(_rt);                                          \
	RT_UNLOCK(_rt);                                                 \
} while (0)

  
                                                                              
                                                                        
                                                                         
                                                                          
                                                             
   
#define NET_CALCULATE_CLOCKSKEW(cc, ic, cu, iu)                         \
	((cc.tv_sec - ic) - (cu - iu))

extern unsigned int rt_verbose;
extern struct radix_node_head *rt_tables[AF_MAX + 1];
extern lck_mtx_t *rnh_lock;
extern uint32_t route_genid_inet;                                        
extern uint32_t route_genid_inet6;                                        
extern int rttrash;
extern unsigned int rte_debug;

struct ifmultiaddr;
struct proc;

extern void route_init(void);
extern void routegenid_update(void);
extern void routegenid_inet_update(void);
extern void routegenid_inet6_update(void);
extern void rt_ifmsg(struct ifnet *);
extern void rt_missmsg(u_char, struct rt_addrinfo *, int, int);
extern void rt_newaddrmsg(u_char, struct ifaddr *, int, struct rtentry *);
extern void rt_newmaddrmsg(u_char, struct ifmultiaddr *);
extern int rt_setgate(struct rtentry *, struct sockaddr *, struct sockaddr *);
extern void set_primary_ifscope(int, unsigned int);
extern unsigned int get_primary_ifscope(int);
extern boolean_t rt_primary_default(struct rtentry *, struct sockaddr *);
extern struct rtentry *rt_lookup(boolean_t, struct sockaddr *,
    struct sockaddr *, struct radix_node_head *, unsigned int);
extern struct rtentry *rt_lookup_coarse(boolean_t, struct sockaddr *,
    struct sockaddr *, struct radix_node_head *);
extern void rtalloc(struct route *);
extern void rtalloc_scoped(struct route *, unsigned int);
extern void rtalloc_ign(struct route *, uint32_t);
extern void rtalloc_scoped_ign(struct route *, uint32_t, unsigned int);
extern struct rtentry *rtalloc1(struct sockaddr *, int, uint32_t);
extern struct rtentry *rtalloc1_scoped(struct sockaddr *, int, uint32_t,
    unsigned int);
extern struct rtentry *rtalloc1_scoped_locked(struct sockaddr *, int,
    uint32_t, unsigned int);
extern void rtfree_locked(struct rtentry *);
extern void rtfree(struct rtentry *);
extern void rtref(struct rtentry *);
  
                                                                         
                                                         
                                                           
   
extern int rtunref(struct rtentry *);
extern void rtsetifa(struct rtentry *, struct ifaddr *);
extern int rtinit(struct ifaddr *, int, int);
extern int rtinit_locked(struct ifaddr *, int, int);
extern int rtioctl(unsigned long, caddr_t, struct proc *);
extern void rtredirect(struct ifnet *, struct sockaddr *, struct sockaddr *,
    struct sockaddr *, int, struct sockaddr *, struct rtentry **);
extern int rtrequest(int, struct sockaddr *,
    struct sockaddr *, struct sockaddr *, int, struct rtentry **);
extern int rtrequest_scoped(int, struct sockaddr *, struct sockaddr *,
    struct sockaddr *, int, struct rtentry **, unsigned int);
extern int rtrequest_locked(int, struct sockaddr *,
    struct sockaddr *, struct sockaddr *, int, struct rtentry **);
extern int rtrequest_scoped_locked(int, struct sockaddr *, struct sockaddr *,
    struct sockaddr *, int, struct rtentry **, unsigned int);
extern void sin_set_ifscope(struct sockaddr *, unsigned int);
extern unsigned int sin_get_ifscope(struct sockaddr *);
extern unsigned int sin6_get_ifscope(struct sockaddr *);
extern void rt_lock(struct rtentry *, boolean_t);
extern void rt_unlock(struct rtentry *);
extern struct sockaddr *rtm_scrub(int, int, struct sockaddr *,
    struct sockaddr *, void *, uint32_t, kauth_cred_t *);
extern boolean_t rt_validate(struct rtentry *);
extern void rt_set_proxy(struct rtentry *, boolean_t);
extern void rt_set_gwroute(struct rtentry *, struct sockaddr *,
    struct rtentry *);
extern void rt_revalidate_gwroute(struct rtentry *, struct rtentry *);
extern errno_t route_to_gwroute(const struct sockaddr *, struct rtentry *,
    struct rtentry **);
extern void rt_setexpire(struct rtentry *, uint64_t);
extern void rt_str(struct rtentry *, char *, uint32_t, char *, uint32_t);
extern const char *rtm2str(int);
extern void route_copyin(struct route *, struct route *, size_t);
extern void route_copyout(struct route *, const struct route *, size_t);
extern boolean_t rt_ifa_is_dst(struct sockaddr *, struct ifaddr *);
extern struct sockaddr *sa_copy(struct sockaddr *, struct sockaddr_storage *,
    unsigned int *);

  
                                                               
                                                           
   
struct route_event {
	struct rtentry *rt;
	  
                                       
                                             
                                               
              
    
	struct rtentry *gwrt;
	union {
		union sockaddr_in_4_6 _rtev_ipaddr;
		struct sockaddr_dl _rtev_lladdr;
		char _rtev_addr_bytes[DLIL_SDLMAXLEN];
	} rt_addr;
	uint32_t route_event_code;
	eventhandler_tag evtag;
};

#define rtev_ipaddr     rt_addr._rtev_ipaddr
#define rtev_lladdr     rt_addr._rtev_lladdr
#define rtev_addr_bytes rt_addr._rtev_addr_bytes

extern void route_event_init(struct route_event *p_route_ev, struct rtentry *rt,
    struct rtentry *gwrt, int route_ev_code);
extern int route_event_walktree(struct radix_node *rn, void *arg);
extern void route_event_enqueue_nwk_wq_entry(struct rtentry *, struct rtentry *,
    uint32_t, eventhandler_tag, boolean_t);
#endif                         
#endif                    
