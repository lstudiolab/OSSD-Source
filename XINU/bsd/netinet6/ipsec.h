                                                                                
                                                            

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

  
                         
   

#ifndef _NETINET6_IPSEC_H_
#define _NETINET6_IPSEC_H_
#include <sys/cdefs.h>
#include <sys/appleapiopts.h>

#include <net/pfkeyv2.h>
#include <uuid/uuid.h>
#ifdef BSD_KERNEL_PRIVATE
#include <netkey/keydb.h>
#include <netinet/ip_var.h>

#include <os/log.h>

                          
extern lck_grp_t         *sadb_stat_mutex_grp;
extern lck_grp_attr_t    *sadb_stat_mutex_grp_attr;
extern lck_attr_t        *sadb_stat_mutex_attr;
extern lck_mtx_t         *sadb_stat_mutex;


#define IPSEC_STAT_INCREMENT(x) \
	OSIncrementAtomic64((SInt64 *)&x)

struct secpolicyaddrrange {
	struct sockaddr_storage start;                                           
	struct sockaddr_storage end;                                            
};

  
                        
                                                                     
                                                                    
                                                                            
   
struct secpolicyindex {
	u_int8_t dir;                                                            
	struct sockaddr_storage src;                               
	struct sockaddr_storage dst;                               
	u_int8_t prefs;                                                    
	u_int8_t prefd;                                                    
	u_int8_t ul_proto;                                       
	ifnet_t internal_if;                                              
	struct secpolicyaddrrange src_range;                                     
	struct secpolicyaddrrange dst_range;                                     
#ifdef notyet
	uid_t uids;
	uid_t uidd;
	gid_t gids;
	gid_t gidd;
#endif
};

                               
struct secpolicy {
	LIST_ENTRY(secpolicy) chain;

	int refcnt;                                          
	struct secpolicyindex spidx;                  
	u_int32_t id;                                                          
	u_int state;                                                
#define IPSEC_SPSTATE_DEAD      0
#define IPSEC_SPSTATE_ALIVE     1

	u_int policy;                                                    
	struct ipsecrequest *req;
	                                        
	                                                

	ifnet_t ipsec_if;                             
	ifnet_t outgoing_if;                                               

	char disabled;                           

	  
                     
                                                                   
                       
                                                                
                                                                 
    
	long created;                                        
	long lastused;                                                        
	long lifetime;                                                       
	long validtime;                                                        
};

                       
struct ipsecrequest {
	struct ipsecrequest *next;
	                               
	                                         
	struct secasindex saidx;                               
	                                                                        
	u_int level;                                            

	struct secpolicy *sp;                           
};

                            
struct inpcbpolicy {
	struct secpolicy *sp_in;
	struct secpolicy *sp_out;
	int priv;                                                
};

                              
struct secspacq {
	LIST_ENTRY(secspacq) chain;

	struct secpolicyindex spidx;

	long created;                             
	int count;                                
	                                                 
};
#endif                         

                                                                            
#define IPSEC_PORT_ANY          0
#define IPSEC_ULPROTO_ANY       255
#define IPSEC_PROTO_ANY         255

                               
                                                                  
#define IPSEC_MODE_ANY          0                           
#define IPSEC_MODE_TRANSPORT    1
#define IPSEC_MODE_TUNNEL       2

  
                                
                                            
                                           
   
#define IPSEC_DIR_ANY           0
#define IPSEC_DIR_INBOUND       1
#define IPSEC_DIR_OUTBOUND      2
#define IPSEC_DIR_MAX           3
#define IPSEC_DIR_INVALID       4

                  
  
                                                                 
                                                           
                                                   
   
#define IPSEC_POLICY_DISCARD    0                              
#define IPSEC_POLICY_NONE       1                                 
#define IPSEC_POLICY_IPSEC      2                     
#define IPSEC_POLICY_ENTRUST    3                                       
#define IPSEC_POLICY_BYPASS     4                                        
#define IPSEC_POLICY_GENERATE   5                                                                            

                             
#define IPSEC_LEVEL_DEFAULT     0                                        
#define IPSEC_LEVEL_USE         1                               
#define IPSEC_LEVEL_REQUIRE     2                        
#define IPSEC_LEVEL_UNIQUE      3                       

#define IPSEC_MANUAL_REQID_MAX  0x3fff
  
                                              
                                             
        
                                             
                                              
                  
                                        
                  
   
#define IPSEC_REPLAYWSIZE  32

                                     
struct ipsecstat {
	u_quad_t in_success __attribute__ ((aligned(8)));                                 
	u_quad_t in_polvio __attribute__ ((aligned(8)));
	                                                   
	u_quad_t in_nosa __attribute__ ((aligned(8)));                                     
	u_quad_t in_inval __attribute__ ((aligned(8)));                                                  
	u_quad_t in_nomem __attribute__ ((aligned(8)));                                                   
	u_quad_t in_badspi __attribute__ ((aligned(8)));                              
	u_quad_t in_ahreplay __attribute__ ((aligned(8)));                              
	u_quad_t in_espreplay __attribute__ ((aligned(8)));                               
	u_quad_t in_ahauthsucc __attribute__ ((aligned(8)));                                 
	u_quad_t in_ahauthfail __attribute__ ((aligned(8)));                                 
	u_quad_t in_espauthsucc __attribute__ ((aligned(8)));                                  
	u_quad_t in_espauthfail __attribute__ ((aligned(8)));                                  
	u_quad_t in_esphist[256] __attribute__ ((aligned(8)));
	u_quad_t in_ahhist[256] __attribute__ ((aligned(8)));
	u_quad_t in_comphist[256] __attribute__ ((aligned(8)));
	u_quad_t out_success __attribute__ ((aligned(8)));                                  
	u_quad_t out_polvio __attribute__ ((aligned(8)));
	                                                    
	u_quad_t out_nosa __attribute__ ((aligned(8)));                                     
	u_quad_t out_inval __attribute__ ((aligned(8)));                                               
	u_quad_t out_nomem __attribute__ ((aligned(8)));                                                   
	u_quad_t out_noroute __attribute__ ((aligned(8)));                         
	u_quad_t out_esphist[256] __attribute__ ((aligned(8)));
	u_quad_t out_ahhist[256] __attribute__ ((aligned(8)));
	u_quad_t out_comphist[256] __attribute__ ((aligned(8)));
};

#define IPSEC_MAX_WAKE_PKT_LEN  100
struct ipsec_wake_pkt_info {
	u_int8_t wake_pkt[IPSEC_MAX_WAKE_PKT_LEN];
	uuid_string_t wake_uuid;
	u_int32_t wake_pkt_spi;
	u_int32_t wake_pkt_seq;
	u_int16_t wake_pkt_len;
};

struct ipsec_wake_pkt_event_data {
	uuid_string_t wake_uuid;
};

#ifdef BSD_KERNEL_PRIVATE
  
                                                 
   
  
                                       
   
#define IPSECCTL_STATS                  1                  
#define IPSECCTL_DEF_POLICY             2
#define IPSECCTL_DEF_ESP_TRANSLEV       3                                    
#define IPSECCTL_DEF_ESP_NETLEV         4                                 
#define IPSECCTL_DEF_AH_TRANSLEV        5                                   
#define IPSECCTL_DEF_AH_NETLEV          6                                
#if 0                               
#define IPSECCTL_INBOUND_CALL_IKE       7
#endif
#define IPSECCTL_AH_CLEARTOS            8
#define IPSECCTL_AH_OFFSETMASK          9
#define IPSECCTL_DFBIT                  10
#define IPSECCTL_ECN                    11
#define IPSECCTL_DEBUG                  12
#define IPSECCTL_ESP_RANDPAD            13
#define IPSECCTL_MAXID                  14

#define IPSECCTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ "ah_cleartos", CTLTYPE_INT }, \
	{ "ah_offsetmask", CTLTYPE_INT }, \
	{ "dfbit", CTLTYPE_INT }, \
	{ "ecn", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
	{ "esp_randpad", CTLTYPE_INT }, \
}

#define IPSEC6CTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "ecn", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
	{ "esp_randpad", CTLTYPE_INT }, \
}

#if defined(__ARM__)
#define IPSEC_IS_P2ALIGNED(p)        IS_P2ALIGNED(p, sizeof (u_int32_t))
#define IPSEC_GET_P2UNALIGNED_OFS(p) (sizeof(u_int32_t) - (((uintptr_t)(p)) & ((uintptr_t)(sizeof(u_int32_t)) - 1)))
#else
#define IPSEC_IS_P2ALIGNED(p)        1
#define IPSEC_GET_P2UNALIGNED_OFS(p) 0
#endif

struct ipsec_output_state {
	int tunneled;
	struct mbuf *m;
	struct route_in6 ro;
	struct sockaddr *dst;
	u_int outgoing_if;
};

struct ipsec_history {
	int ih_proto;
	u_int32_t ih_spi;
};

extern int ipsec_debug;

extern struct ipsecstat ipsecstat;
extern struct secpolicy ip4_def_policy;
extern int ip4_esp_trans_deflev;
extern int ip4_esp_net_deflev;
extern int ip4_ah_trans_deflev;
extern int ip4_ah_net_deflev;
extern int ip4_ah_cleartos;
extern int ip4_ah_offsetmask;
extern int ip4_ipsec_dfbit;
extern int ip4_ipsec_ecn;
extern int ip4_esp_randpad;

extern bool ipsec_save_wake_pkt;

#define _ipsec_log(level, fmt, ...) do {                            \
	os_log_type_t type;                                         \
	switch (level) {                                            \
	default:                                                    \
	        type = OS_LOG_TYPE_DEFAULT;                         \
	        break;                                              \
	case LOG_INFO:                                              \
	        type = OS_LOG_TYPE_INFO;                            \
	        break;                                              \
	case LOG_DEBUG:                                             \
	        type = OS_LOG_TYPE_DEBUG;                           \
	        break;                                              \
	case LOG_ERR:                                               \
	        type = OS_LOG_TYPE_ERROR;                           \
	        break;                                              \
	}                                                           \
	os_log_with_type(OS_LOG_DEFAULT, type, fmt, ##__VA_ARGS__); \
} while (0)

#define ipseclog(x)     do { if (ipsec_debug != 0) _ipsec_log x; } while (0)

extern struct secpolicy *ipsec4_getpolicybysock(struct mbuf *, u_int8_t,
    struct socket *, int *);
extern struct secpolicy *ipsec4_getpolicybyaddr(struct mbuf *, u_int8_t, int,
    int *);
extern int ipsec4_getpolicybyinterface(struct mbuf *, u_int8_t, int *,
    struct ip_out_args *, struct secpolicy **);

extern u_int ipsec_get_reqlevel(struct ipsecrequest *);

struct inpcb;
extern int ipsec_init_policy(struct socket *so, struct inpcbpolicy **);
extern int ipsec_copy_policy(struct inpcbpolicy *, struct inpcbpolicy *);
extern u_int ipsec_get_reqlevel(struct ipsecrequest *);

extern int ipsec4_set_policy(struct inpcb *inp, int optname,
    caddr_t request, size_t len, int priv);
extern int ipsec4_delete_pcbpolicy(struct inpcb *);
extern int ipsec4_in_reject_so(struct mbuf *, struct socket *);
extern int ipsec4_in_reject(struct mbuf *, struct inpcb *);

struct secas;
struct tcpcb;
extern int ipsec_chkreplay(u_int32_t, struct secasvar *, u_int8_t);
extern int ipsec_updatereplay(u_int32_t, struct secasvar *, u_int8_t);

extern size_t ipsec4_hdrsiz(struct mbuf *, u_int8_t, struct inpcb *);
extern size_t ipsec_hdrsiz_tcp(struct tcpcb *);
extern size_t ipsec_hdrsiz(struct secpolicy *);

struct ip;
extern const char *ipsec4_logpacketstr(struct ip *, u_int32_t);
extern const char *ipsec_logsastr(struct secasvar *);

extern void ipsec_dumpmbuf(struct mbuf *);

extern int ipsec4_interface_output(struct ipsec_output_state *state, ifnet_t interface);
extern int ipsec4_output(struct ipsec_output_state *, struct secpolicy *, int);
#if INET
extern struct mbuf * ipsec4_splithdr(struct mbuf *);
extern int ipsec4_encapsulate(struct mbuf *, struct secasvar *);
#endif
extern struct mbuf * ipsec6_splithdr(struct mbuf *);
extern int ipsec6_encapsulate(struct mbuf *, struct secasvar *);
extern int ipsec4_tunnel_validate(struct mbuf *, int, u_int, struct secasvar *, sa_family_t *);
extern struct mbuf *ipsec_copypkt(struct mbuf *);
extern void ipsec_delaux(struct mbuf *);
extern int ipsec_setsocket(struct mbuf *, struct socket *);
extern struct socket *ipsec_getsocket(struct mbuf *);
extern int ipsec_addhist(struct mbuf *, int, u_int32_t);
extern struct ipsec_history *ipsec_gethist(struct mbuf *, int *);
extern void ipsec_clearhist(struct mbuf *);
extern void ipsec_monitor_sleep_wake(void);
extern void ipsec_save_wake_packet(struct mbuf *, u_int32_t, u_int32_t);
#endif                         

#ifndef KERNEL
__BEGIN_DECLS
extern caddr_t ipsec_set_policy(char *, int);
extern int ipsec_get_policylen(caddr_t);
extern char *ipsec_dump_policy(caddr_t, char *);

extern const char *ipsec_strerror(void);
__END_DECLS
#endif             

#endif                         
