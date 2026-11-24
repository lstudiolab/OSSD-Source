/*
 * Copyright (c) 2008-2020 Apple Inc. All rights reserved.
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

  
                                  
   

#include <machine/endian.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/mcache.h>

#include <kern/locks.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>

#include <netinet/ip6.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>

#include <net/pfkeyv2.h>
#include <netkey/keydb.h>
#include <netkey/key.h>
#include <netkey/keysock.h>
#include <netkey/key_debug.h>
#include <stdarg.h>
#include <libkern/crypto/rand.h>

#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
#include <netinet6/ah.h>
#include <netinet6/ah6.h>
#if IPSEC_ESP
#include <netinet6/esp.h>
#include <netinet6/esp6.h>
#endif


                
#include <sys/random.h>

#include <net/net_osdep.h>

#define FULLMASK        0xff

lck_grp_t         *sadb_mutex_grp;
lck_grp_attr_t    *sadb_mutex_grp_attr;
lck_attr_t        *sadb_mutex_attr;
decl_lck_mtx_data(, sadb_mutex_data);
lck_mtx_t         *sadb_mutex = &sadb_mutex_data;

lck_grp_t         *pfkey_stat_mutex_grp;
lck_grp_attr_t    *pfkey_stat_mutex_grp_attr;
lck_attr_t        *pfkey_stat_mutex_attr;
decl_lck_mtx_data(, pfkey_stat_mutex_data);
lck_mtx_t         *pfkey_stat_mutex = &pfkey_stat_mutex_data;

  
                                 
                                                                            
                                                                            
                               
                                                                    
                                                                           
                                                                        
                                                                     
   

u_int32_t key_debug_level = 0;                                
static int key_timehandler_running = 0;
static u_int key_spi_trycnt = 1000;
static u_int32_t key_spi_minval = 0x100;
static u_int32_t key_spi_maxval = 0x0fffffff;            
static u_int32_t policy_id = 0;
static u_int key_int_random = 60;                                               
static u_int key_larval_lifetime = 30;                                          
static int key_blockacq_count = 10;                                            
static int key_blockacq_lifetime = 20;                                          
static int key_preferred_oldsa = 0;                                              
__private_extern__ int natt_keepalive_interval = 20;                                          
static u_int32_t ipsec_policy_count = 0;
static u_int32_t ipsec_sav_count = 0;

static u_int32_t acq_seq = 0;
static int key_tick_init_random = 0;
static u_int64_t up_time = 0;
__private_extern__ u_int64_t natt_now = 0;

static LIST_HEAD(_sptree, secpolicy) sptree[IPSEC_DIR_MAX];              
static LIST_HEAD(_sahtree, secashead) sahtree;                           
static LIST_HEAD(_regtree, secreg) regtree[SADB_SATYPE_MAX + 1];
static LIST_HEAD(_custom_sahtree, secashead) custom_sahtree;
                   

#define SPIHASHSIZE     128
#define SPIHASH(x)      (((x) ^ ((x) >> 16)) % SPIHASHSIZE)
static LIST_HEAD(_spihash, secasvar) spihash[SPIHASHSIZE];

#ifndef IPSEC_NONBLOCK_ACQUIRE
static LIST_HEAD(_acqtree, secacq) acqtree;                                 
#endif
static LIST_HEAD(_spacqtree, secspacq) spacqtree;                              

struct key_cb key_cb;

                          
static const u_int saorder_state_valid_prefer_old[] = {
	SADB_SASTATE_DYING, SADB_SASTATE_MATURE,
};
static const u_int saorder_state_valid_prefer_new[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING,
};
static const u_int saorder_state_alive[] = {
	                 
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING, SADB_SASTATE_LARVAL
};
static const u_int saorder_state_any[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING,
	SADB_SASTATE_LARVAL, SADB_SASTATE_DEAD
};

static const int minsize[] = {
	sizeof(struct sadb_msg),                               
	sizeof(struct sadb_sa),                          
	sizeof(struct sadb_lifetime),                                  
	sizeof(struct sadb_lifetime),                               
	sizeof(struct sadb_lifetime),                               
	sizeof(struct sadb_address),                              
	sizeof(struct sadb_address),                              
	sizeof(struct sadb_address),                                
	sizeof(struct sadb_key),                               
	sizeof(struct sadb_key),                                  
	sizeof(struct sadb_ident),                                 
	sizeof(struct sadb_ident),                                 
	sizeof(struct sadb_sens),                                 
	sizeof(struct sadb_prop),                              
	sizeof(struct sadb_supported),                               
	sizeof(struct sadb_supported),                                  
	sizeof(struct sadb_spirange),                          
	0,                                                        
	sizeof(struct sadb_x_policy),                          
	sizeof(struct sadb_x_sa2),                      
	sizeof(struct sadb_session_id),                          
	sizeof(struct sadb_sastat),                          
	sizeof(struct sadb_x_ipsecif),                         
	sizeof(struct sadb_address),                                         
	sizeof(struct sadb_address),                                       
	sizeof(struct sadb_address),                                         
	sizeof(struct sadb_address),                                       
	sizeof(struct sadb_address),                                      
	sizeof(struct sadb_address),                                      
	sizeof(struct sadb_x_ipsecif),                                  
};
static const int maxsize[] = {
	sizeof(struct sadb_msg),                               
	sizeof(struct sadb_sa_2),                                
	sizeof(struct sadb_lifetime),                                  
	sizeof(struct sadb_lifetime),                               
	sizeof(struct sadb_lifetime),                               
	0,                                                        
	0,                                                        
	0,                                                          
	0,                                                     
	0,                                                        
	0,                                                         
	0,                                                         
	0,                                                        
	0,                                                     
	0,                                                           
	0,                                                              
	sizeof(struct sadb_spirange),                          
	0,                                                        
	0,                                                     
	sizeof(struct sadb_x_sa2),                      
	0,                                                       
	0,                                                   
	sizeof(struct sadb_x_ipsecif),                         
	0,                                               
	0,                                                 
	0,                                               
	0,                                                 
	0,                                                
	0,                                                
	sizeof(struct sadb_x_ipsecif),                                 
};

static int ipsec_esp_keymin = 256;
static int ipsec_esp_auth = 0;
static int ipsec_ah_keymin = 128;

SYSCTL_DECL(_net_key);
                                       
SYSCTL_INT(_net_key, KEYCTL_DEBUG_LEVEL, debug, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_debug_level, 0, "");


                                                      
SYSCTL_INT(_net_key, KEYCTL_SPI_TRY, spi_trycnt, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_spi_trycnt, 0, "");

                                                  
SYSCTL_INT(_net_key, KEYCTL_SPI_MIN_VALUE, spi_minval, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_spi_minval, 0, "");

                                                  
SYSCTL_INT(_net_key, KEYCTL_SPI_MAX_VALUE, spi_maxval, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_spi_maxval, 0, "");

                                     
SYSCTL_INT(_net_key, KEYCTL_RANDOM_INT, int_random, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_int_random, 0, "");

                                                          
SYSCTL_INT(_net_key, KEYCTL_LARVAL_LIFETIME, larval_lifetime, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_larval_lifetime, 0, "");

                                                       
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_COUNT, blockacq_count, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_blockacq_count, 0, "");

                                                                                
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_LIFETIME, blockacq_lifetime, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_blockacq_lifetime, 0, "");

              
SYSCTL_INT(_net_key, KEYCTL_ESP_AUTH, esp_auth, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &ipsec_esp_auth, 0, "");

                            
SYSCTL_INT(_net_key, KEYCTL_ESP_KEYMIN, esp_keymin, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &ipsec_esp_keymin, 0, "");

                           
SYSCTL_INT(_net_key, KEYCTL_AH_KEYMIN, ah_keymin, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &ipsec_ah_keymin, 0, "");

                                        
SYSCTL_INT(_net_key, KEYCTL_PREFERED_OLDSA, prefered_oldsa, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &key_preferred_oldsa, 0, "");

                                                          
SYSCTL_INT(_net_key, KEYCTL_NATT_KEEPALIVE_INTERVAL, natt_keepalive_interval, CTLFLAG_RW | CTLFLAG_LOCKED, \
        &natt_keepalive_interval, 0, "");

                       
SYSCTL_STRUCT(_net_key, KEYCTL_PFKEYSTAT, pfkeystat, CTLFLAG_RD | CTLFLAG_LOCKED, \
        &pfkeystat, pfkeystat, "");

#ifndef LIST_FOREACH
#define LIST_FOREACH(elm, head, field)                                     \
for (elm = LIST_FIRST(head); elm; elm = LIST_NEXT(elm, field))
#endif
#define __LIST_CHAINED(elm) \
(!((elm)->chain.le_next == NULL && (elm)->chain.le_prev == NULL))
#define LIST_INSERT_TAIL(head, elm, type, field) \
do {\
struct type *curelm = LIST_FIRST(head); \
if (curelm == NULL) {\
LIST_INSERT_HEAD(head, elm, field); \
} else { \
while (LIST_NEXT(curelm, field)) \
curelm = LIST_NEXT(curelm, field);\
LIST_INSERT_AFTER(curelm, elm, field);\
}\
} while (0)

#define KEY_CHKSASTATE(head, sav, name) \
do { \
if ((head) != (sav)) {                                          \
ipseclog((LOG_DEBUG, "%s: state mismatched (TREE=%d SA=%d)\n", \
(name), (head), (sav)));                        \
continue;                                               \
}                                                               \
} while (0)

#define KEY_CHKSPDIR(head, sp, name) \
do { \
if ((head) != (sp)) {                                           \
ipseclog((LOG_DEBUG, "%s: direction mismatched (TREE=%d SP=%d), " \
"anyway continue.\n",                           \
(name), (head), (sp)));                         \
}                                                               \
} while (0)

#if 1
#define KMALLOC_WAIT(p, t, n)                                                     \
((p) = (t) _MALLOC((n), M_SECA, M_WAITOK))
#define KMALLOC_NOWAIT(p, t, n)                                              \
((p) = (t) _MALLOC((n), M_SECA, M_NOWAIT))
#define KFREE(p)                                                             \
_FREE((caddr_t)(p), M_SECA);
#else
#define KMALLOC_WAIT(p, t, n) \
do { \
((p) = (t)_MALLOC((u_int32_t)(n), M_SECA, M_WAITOK));             \
printf("%s %d: %p <- KMALLOC_WAIT(%s, %d)\n",                             \
__FILE__, __LINE__, (p), #t, n);                             \
} while (0)
#define KMALLOC_NOWAIT(p, t, n) \
do { \
((p) = (t)_MALLOC((u_int32_t)(n), M_SECA, M_NOWAIT));             \
printf("%s %d: %p <- KMALLOC_NOWAIT(%s, %d)\n",                             \
__FILE__, __LINE__, (p), #t, n);                             \
} while (0)

#define KFREE(p)                                                             \
do {                                                                 \
printf("%s %d: %p -> KFREE()\n", __FILE__, __LINE__, (p));   \
_FREE((caddr_t)(p), M_SECA);                                  \
} while (0)
#endif

  
                                             
                                                               
   
#define KEY_SETSECSPIDX(_dir, s, d, ps, pd, ulp, ifp, s_s, s_e, d_s, d_e, idx) \
do { \
bzero((idx), sizeof(struct secpolicyindex));                         \
(idx)->dir = (_dir);                                                 \
(idx)->prefs = (ps);                                                 \
(idx)->prefd = (pd);                                                 \
(idx)->ul_proto = (ulp);                                             \
(idx)->internal_if = (ifp);                                          \
if (s) bcopy((s), &(idx)->src, ((struct sockaddr *)(s))->sa_len);    \
if (d) bcopy((d), &(idx)->dst, ((struct sockaddr *)(d))->sa_len);    \
if (s_s) bcopy((s_s), &(idx)->src_range.start, ((struct sockaddr *)(s_s))->sa_len);   \
if (s_e) bcopy((s_e), &(idx)->src_range.end, ((struct sockaddr *)(s_e))->sa_len);     \
if (d_s) bcopy((d_s), &(idx)->dst_range.start, ((struct sockaddr *)(d_s))->sa_len);   \
if (d_e) bcopy((d_e), &(idx)->dst_range.end, ((struct sockaddr *)(d_e))->sa_len);     \
} while (0)

  
                                         
                                                                
   
#define KEY_SETSECASIDX(p, m, r, s, d, ifi, idx) \
do { \
bzero((idx), sizeof(struct secasindex));                             \
(idx)->proto = (p);                                                  \
(idx)->mode = (m);                                                   \
(idx)->reqid = (r);                                                  \
bcopy((s), &(idx)->src, ((const struct sockaddr *)(s))->sa_len);           \
bcopy((d), &(idx)->dst, ((const struct sockaddr *)(d))->sa_len);           \
(idx)->ipsec_ifindex = (ifi);                                                                           \
} while (0)

                    
struct _keystat {
	u_int32_t getspi_count;                                                 
} keystat;

struct sadb_msghdr {
	struct sadb_msg *msg;
	struct sadb_ext *ext[SADB_EXT_MAX + 1];
	int extoff[SADB_EXT_MAX + 1];
	int extlen[SADB_EXT_MAX + 1];
};

static struct secpolicy *__key_getspbyid(u_int32_t id);
static struct secasvar *key_do_allocsa_policy(struct secashead *, u_int, u_int16_t);
static int key_do_get_translated_port(struct secashead *, struct secasvar *, u_int);
static void key_delsp(struct secpolicy *);
static struct secpolicy *key_getsp(struct secpolicyindex *);
static u_int16_t key_newreqid(void);
static struct mbuf *key_gather_mbuf(struct mbuf *,
    const struct sadb_msghdr *, int, int, int *);
static int key_spdadd(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static u_int32_t key_getnewspid(void);
static int key_spddelete(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spddelete2(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spdenable(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spddisable(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spdget(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spdflush(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_spddump(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static struct mbuf *key_setdumpsp(struct secpolicy *,
    u_int8_t, u_int32_t, u_int32_t);
static u_int key_getspreqmsglen(struct secpolicy *);
static int key_spdexpire(struct secpolicy *);
static struct secashead *key_newsah(struct secasindex *, ifnet_t, u_int, u_int8_t, u_int16_t);
static struct secasvar *key_newsav(struct mbuf *,
    const struct sadb_msghdr *, struct secashead *, int *,
    struct socket *);
static struct secashead *key_getsah(struct secasindex *, u_int16_t);
static struct secasvar *key_checkspidup(struct secasindex *, u_int32_t);
static void key_setspi __P((struct secasvar *, u_int32_t));
static struct secasvar *key_getsavbyspi(struct secashead *, u_int32_t);
static int key_setsaval(struct secasvar *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_mature(struct secasvar *);
static struct mbuf *key_setdumpsa(struct secasvar *, u_int8_t,
    u_int8_t, u_int32_t, u_int32_t);
static struct mbuf *key_setsadbmsg(u_int8_t, u_int16_t, u_int8_t,
    u_int32_t, pid_t, u_int16_t);
static struct mbuf *key_setsadbsa(struct secasvar *);
static struct mbuf *key_setsadbaddr(u_int16_t,
    struct sockaddr *, size_t, u_int8_t);
static struct mbuf *key_setsadbipsecif(ifnet_t, ifnet_t, ifnet_t, u_int8_t);
static struct mbuf *key_setsadbxsa2(u_int8_t, u_int32_t, u_int32_t, u_int16_t);
static struct mbuf *key_setsadbxpolicy(u_int16_t, u_int8_t,
    u_int32_t);
static void *key_newbuf(const void *, u_int);
static int key_ismyaddr6(struct sockaddr_in6 *);
static void key_update_natt_keepalive_timestamp(struct secasvar *, struct secasvar *);

                              
#define CMP_HEAD        0x1                               
#define CMP_PORT        0x2                                          
#define CMP_REQID       0x4                                    
#define CMP_MODE        0x8                               
#define CMP_EXACTLY     0xF                        
static int key_cmpsaidx(struct secasindex *, struct secasindex *, int);

static int key_cmpspidx_exactly(struct secpolicyindex *,
    struct secpolicyindex *);
static int key_cmpspidx_withmask(struct secpolicyindex *,
    struct secpolicyindex *);
static int key_sockaddrcmp(struct sockaddr *, struct sockaddr *, int);
static int key_is_addr_in_range(struct sockaddr_storage *, struct secpolicyaddrrange *);
static int key_bbcmp(caddr_t, caddr_t, u_int);
static void key_srandom(void);
static u_int8_t key_satype2proto(u_int8_t);
static u_int8_t key_proto2satype(u_int16_t);

static int key_getspi(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static u_int32_t key_do_getnewspi(struct sadb_spirange *, struct secasindex *);
static int key_update(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_add(struct socket *, struct mbuf *, const struct sadb_msghdr *);
static struct mbuf *key_getmsgbuf_x1(struct mbuf *, const struct sadb_msghdr *);
static int key_delete(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_get(struct socket *, struct mbuf *, const struct sadb_msghdr *);

static void key_getcomb_setlifetime(struct sadb_comb *);
#if IPSEC_ESP
static struct mbuf *key_getcomb_esp(void);
#endif
static struct mbuf *key_getcomb_ah(void);
static struct mbuf *key_getprop(const struct secasindex *);

static int key_acquire(struct secasindex *, struct secpolicy *);
#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *key_newacq(struct secasindex *);
static struct secacq *key_getacq(struct secasindex *);
static struct secacq *key_getacqbyseq(u_int32_t);
#endif
static struct secspacq *key_newspacq(struct secpolicyindex *);
static struct secspacq *key_getspacq(struct secpolicyindex *);
static int key_acquire2(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_register(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_expire(struct secasvar *);
static int key_flush(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_dump(struct socket *, struct mbuf *, const struct sadb_msghdr *);
static int key_promisc(struct socket *, struct mbuf *,
    const struct sadb_msghdr *);
static int key_senderror(struct socket *, struct mbuf *, int);
static int key_validate_ext(const struct sadb_ext *, int);
static int key_align(struct mbuf *, struct sadb_msghdr *);
static struct mbuf *key_alloc_mbuf(int);
static int key_getsastat(struct socket *, struct mbuf *, const struct sadb_msghdr *);
static int key_migrate(struct socket *, struct mbuf *, const struct sadb_msghdr *);
static void bzero_keys(const struct sadb_msghdr *);

extern int ipsec_bypass;
extern int esp_udp_encap_port;
int ipsec_send_natt_keepalive(struct secasvar *sav);
bool ipsec_fill_offload_frame(ifnet_t ifp, struct secasvar *sav, struct ifnet_keepalive_offload_frame *frame, size_t frame_data_offset);

void key_init(struct protosw *, struct domain *);

  
              
                                                                        
  
   
void
key_init(struct protosw *pp, struct domain *dp)
{
	static int key_initialized = 0;
	int i;

	VERIFY((pp->pr_flags & (PR_INITIALIZED | PR_ATTACHED)) == PR_ATTACHED);

	_CASSERT(PFKEY_ALIGN8(sizeof(struct sadb_msg)) <= _MHLEN);
	_CASSERT(MAX_REPLAY_WINDOWS == MBUF_TC_MAX);

	if (key_initialized) {
		return;
	}
	key_initialized = 1;

	sadb_mutex_grp_attr = lck_grp_attr_alloc_init();
	sadb_mutex_grp = lck_grp_alloc_init("sadb", sadb_mutex_grp_attr);
	sadb_mutex_attr = lck_attr_alloc_init();

	lck_mtx_init(sadb_mutex, sadb_mutex_grp, sadb_mutex_attr);

	pfkey_stat_mutex_grp_attr = lck_grp_attr_alloc_init();
	pfkey_stat_mutex_grp = lck_grp_alloc_init("pfkey_stat", pfkey_stat_mutex_grp_attr);
	pfkey_stat_mutex_attr = lck_attr_alloc_init();

	lck_mtx_init(pfkey_stat_mutex, pfkey_stat_mutex_grp, pfkey_stat_mutex_attr);

	for (i = 0; i < SPIHASHSIZE; i++) {
		LIST_INIT(&spihash[i]);
	}

	raw_init(pp, dp);

	bzero((caddr_t)&key_cb, sizeof(key_cb));

	for (i = 0; i < IPSEC_DIR_MAX; i++) {
		LIST_INIT(&sptree[i]);
	}
	ipsec_policy_count = 0;

	LIST_INIT(&sahtree);
	LIST_INIT(&custom_sahtree);

	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		LIST_INIT(&regtree[i]);
	}
	ipsec_sav_count = 0;

#ifndef IPSEC_NONBLOCK_ACQUIRE
	LIST_INIT(&acqtree);
#endif
	LIST_INIT(&spacqtree);

	                    
#if INET
	ip4_def_policy.policy = IPSEC_POLICY_NONE;
	ip4_def_policy.refcnt++;                              
#endif
	ip6_def_policy.policy = IPSEC_POLICY_NONE;
	ip6_def_policy.refcnt++;                              

	key_timehandler_running = 0;

	                               
	keystat.getspi_count = 1;

	esp_init();
#ifndef __APPLE__
	printf("IPsec: Initialized Security Association Processing.\n");
#endif
}

static void
key_start_timehandler(void)
{
	                                 
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	if (key_timehandler_running == 0) {
		key_timehandler_running = 1;
		(void)timeout((void *)key_timehandler, (void *)0, hz);
	}

	                               
	if (ipsec_bypass != 0) {
		ipsec_bypass = 0;
	}
}

                                 
  
                                                  
                                
                       
                                        
   
struct secpolicy *
key_allocsp(
	struct secpolicyindex *spidx,
	u_int dir)
{
	struct secpolicy *sp;
	struct timeval tv;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);
	                  
	if (spidx == NULL) {
		panic("key_allocsp: NULL pointer is passed.\n");
	}

	                     
	switch (dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		panic("key_allocsp: Invalid direction is passed.\n");
	}

	                    
	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
	    printf("*** objects\n");
	    kdebug_secpolicyindex(spidx));

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sp, &sptree[dir], chain) {
		KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		    printf("*** in SPD\n");
		    kdebug_secpolicyindex(&sp->spidx));

		if (sp->state == IPSEC_SPSTATE_DEAD) {
			continue;
		}

		                                     
		if (sp->disabled > 0) {
			continue;
		}

		                                            
                               
		if (spidx->internal_if != NULL
		    && (sp->spidx.internal_if == NULL || sp->ipsec_if == NULL)) {
			continue;
		}

		if (key_cmpspidx_withmask(&sp->spidx, spidx)) {
			goto found;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return NULL;

found:

	                       
	microtime(&tv);
	sp->lastused = tv.tv_sec;
	sp->refcnt++;
	lck_mtx_unlock(sadb_mutex);

	                  
	KEY_CHKSPDIR(sp->spidx.dir, dir, "key_allocsp");
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP key_allocsp cause refcnt++:%d SP:0x%llx\n",
	    sp->refcnt, (uint64_t)VM_KERNEL_ADDRPERM(sp)));
	return sp;
}

  
                                                               
           
   
struct secpolicy *
key_gettunnel(
	struct sockaddr *osrc,
	struct sockaddr *odst,
	struct sockaddr *isrc,
	struct sockaddr *idst)
{
	struct secpolicy *sp;
	const int dir = IPSEC_DIR_INBOUND;
	struct timeval tv;
	struct ipsecrequest *r1, *r2, *p;
	struct sockaddr *os, *od, *is, *id;
	struct secpolicyindex spidx;

	if (isrc->sa_family != idst->sa_family) {
		ipseclog((LOG_ERR, "protocol family mismatched %d != %d\n.",
		    isrc->sa_family, idst->sa_family));
		return NULL;
	}

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sp, &sptree[dir], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD) {
			continue;
		}

		r1 = r2 = NULL;
		for (p = sp->req; p; p = p->next) {
			if (p->saidx.mode != IPSEC_MODE_TUNNEL) {
				continue;
			}

			r1 = r2;
			r2 = p;

			if (!r1) {
				                                          
				spidx = sp->spidx;
				if (isrc->sa_len > sizeof(spidx.src) ||
				    idst->sa_len > sizeof(spidx.dst)) {
					continue;
				}
				bcopy(isrc, &spidx.src, isrc->sa_len);
				bcopy(idst, &spidx.dst, idst->sa_len);
				if (!key_cmpspidx_withmask(&sp->spidx, &spidx)) {
					continue;
				}
			} else {
				is = (struct sockaddr *)&r1->saidx.src;
				id = (struct sockaddr *)&r1->saidx.dst;
				if (key_sockaddrcmp(is, isrc, 0) ||
				    key_sockaddrcmp(id, idst, 0)) {
					continue;
				}
			}

			os = (struct sockaddr *)&r2->saidx.src;
			od = (struct sockaddr *)&r2->saidx.dst;
			if (key_sockaddrcmp(os, osrc, 0) ||
			    key_sockaddrcmp(od, odst, 0)) {
				continue;
			}

			goto found;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return NULL;

found:
	microtime(&tv);
	sp->lastused = tv.tv_sec;
	sp->refcnt++;
	lck_mtx_unlock(sadb_mutex);
	return sp;
}

struct secasvar *
key_alloc_outbound_sav_for_interface(ifnet_t interface, int family,
    struct sockaddr *src,
    struct sockaddr *dst)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int stateidx;
	u_int state;
	const u_int *saorder_state_valid;
	int arraysize;
	struct sockaddr_in *sin;
	u_int16_t dstport;
	bool strict = true;

	if (interface == NULL) {
		return NULL;
	}

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	lck_mtx_lock(sadb_mutex);

	do {
		LIST_FOREACH(sah, &sahtree, chain) {
			if (sah->state == SADB_SASTATE_DEAD) {
				continue;
			}
			if (sah->ipsec_if == interface &&
			    (family == AF_INET6 || family == AF_INET) &&
			    sah->dir == IPSEC_DIR_OUTBOUND) {
				if (strict &&
				    sah->saidx.mode == IPSEC_MODE_TRANSPORT &&
				    src != NULL && dst != NULL) {
					                                        
					if (key_sockaddrcmp((struct sockaddr *)&sah->saidx.src, src, 0) != 0) {
						                       
						continue;
					}

					if (key_sockaddrcmp((struct sockaddr *)&sah->saidx.dst, dst, 0) != 0) {
						                            
						continue;
					}
				}

				                                                                                   
				if (key_preferred_oldsa) {
					saorder_state_valid = saorder_state_valid_prefer_old;
					arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
				} else {
					saorder_state_valid = saorder_state_valid_prefer_new;
					arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
				}

				sin = (struct sockaddr_in *)&sah->saidx.dst;
				dstport = sin->sin_port;
				if (sah->saidx.mode == IPSEC_MODE_TRANSPORT) {
					sin->sin_port = IPSEC_PORT_ANY;
				}

				for (stateidx = 0; stateidx < arraysize; stateidx++) {
					state = saorder_state_valid[stateidx];
					sav = key_do_allocsa_policy(sah, state, dstport);
					if (sav != NULL) {
						lck_mtx_unlock(sadb_mutex);
						return sav;
					}
				}

				break;
			}
		}
		if (strict) {
			                                                       
			strict = false;
		} else {
			                                          
			break;
		}
	} while (true);

	lck_mtx_unlock(sadb_mutex);
	return NULL;
}

  
                                                   
                                                                  
                                    
                                                                    
   
int
key_checkrequest(
	struct ipsecrequest *isr,
	struct secasindex *saidx,
	struct secasvar **sav)
{
	u_int level;
	int error;
	struct sockaddr_in *sin;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	*sav = NULL;

	                  
	if (isr == NULL || saidx == NULL) {
		panic("key_checkrequest: NULL pointer is passed.\n");
	}

	                
	switch (saidx->mode) {
	case IPSEC_MODE_TRANSPORT:
	case IPSEC_MODE_TUNNEL:
		break;
	case IPSEC_MODE_ANY:
	default:
		panic("key_checkrequest: Invalid policy defined.\n");
	}

	                       
	level = ipsec_get_reqlevel(isr);


	  
                                                               
                                                                         
    
	if (*sav == NULL) {
		*sav = key_allocsa_policy(saidx);
	}

	                       
	if (*sav != NULL) {
		return 0;
	}

	                  
   
                                                                
                        
    
	if (saidx->mode == IPSEC_MODE_TRANSPORT) {
		sin = (struct sockaddr_in *)&saidx->dst;
		sin->sin_port = IPSEC_PORT_ANY;
	}
	if ((error = key_acquire(saidx, isr->sp)) != 0) {
		                            
		ipseclog((LOG_DEBUG, "key_checkrequest: error %d returned "
		    "from key_acquire.\n", error));
		return error;
	}

	return level == IPSEC_LEVEL_REQUIRE ? ENOENT : 0;
}

  
                                             
                                        
                        
                                        
   
u_int32_t sah_search_calls = 0;
u_int32_t sah_search_count = 0;
struct secasvar *
key_allocsa_policy(
	struct secasindex *saidx)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int stateidx, state;
	const u_int *saorder_state_valid;
	int arraysize;
	struct sockaddr_in *sin;
	u_int16_t       dstport;

	lck_mtx_lock(sadb_mutex);
	sah_search_calls++;
	LIST_FOREACH(sah, &sahtree, chain) {
		sah_search_count++;
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, saidx, CMP_MODE | CMP_REQID)) {
			goto found;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return NULL;

found:

	  
                                                  
                                   
    
	if (key_preferred_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}


	sin = (struct sockaddr_in *)&saidx->dst;
	dstport = sin->sin_port;
	if (saidx->mode == IPSEC_MODE_TRANSPORT) {
		sin->sin_port = IPSEC_PORT_ANY;
	}

	for (stateidx = 0; stateidx < arraysize; stateidx++) {
		state = saorder_state_valid[stateidx];

		sav = key_do_allocsa_policy(sah, state, dstport);
		if (sav != NULL) {
			lck_mtx_unlock(sadb_mutex);
			return sav;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return NULL;
}

static void
key_send_delete(struct secasvar *sav)
{
	struct mbuf *m, *result;
	u_int8_t satype;

	key_sa_chgstate(sav, SADB_SASTATE_DEAD);

	if ((satype = key_proto2satype(sav->sah->saidx.proto)) == 0) {
		panic("key_do_allocsa_policy: invalid proto is passed.\n");
	}

	m = key_setsadbmsg(SADB_DELETE, 0,
	    satype, 0, 0, (u_int16_t)(sav->refcnt - 1));
	if (!m) {
		goto msgfail;
	}
	result = m;

	                                   
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    (struct sockaddr *)&sav->sah->saidx.src,
	    sav->sah->saidx.src.ss_len << 3,
	        IPSEC_ULPROTO_ANY);
	if (!m) {
		goto msgfail;
	}
	m_cat(result, m);

	                                   
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    (struct sockaddr *)&sav->sah->saidx.dst,
	    sav->sah->saidx.src.ss_len << 3,
	        IPSEC_ULPROTO_ANY);
	if (!m) {
		goto msgfail;
	}
	m_cat(result, m);

	                         
	m = key_setsadbsa(sav);
	if (!m) {
		goto msgfail;
	}
	m_cat(result, m);

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result,
		    sizeof(struct sadb_msg));
		if (result == NULL) {
			goto msgfail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	VERIFY(PFKEY_UNIT64(result->m_pkthdr.len) <= UINT16_MAX);
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	if (key_sendup_mbuf(NULL, result,
	    KEY_SENDUP_REGISTERED)) {
		goto msgfail;
	}
msgfail:
	key_freesav(sav, KEY_SADB_LOCKED);
}

  
                                                          
                                  
       
                   
                                   
   
static struct secasvar *
key_do_allocsa_policy(
	struct secashead *sah,
	u_int state,
	u_int16_t dstport)
{
	struct secasvar *sav, *nextsav, *candidate, *natt_candidate, *no_natt_candidate, *d;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                
	candidate = NULL;
	natt_candidate = NULL;
	no_natt_candidate = NULL;

	for (sav = LIST_FIRST(&sah->savtree[state]);
	    sav != NULL;
	    sav = nextsav) {
		nextsav = LIST_NEXT(sav, chain);

		                  
		KEY_CHKSASTATE(sav->state, state, "key_do_allocsa_policy");

		if (sah->saidx.mode == IPSEC_MODE_TUNNEL && dstport &&
		    ((sav->flags & SADB_X_EXT_NATT) != 0) &&
		    ntohs(dstport) != sav->remote_ike_port) {
			continue;
		}

		if (sah->saidx.mode == IPSEC_MODE_TRANSPORT &&
		    ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0) &&
		    ntohs(dstport) != sav->remote_ike_port) {
			continue;                                                     
		}
		if ((sah->saidx.mode == IPSEC_MODE_TUNNEL &&
		    ((sav->flags & SADB_X_EXT_NATT) != 0)) ||
		    (sah->saidx.mode == IPSEC_MODE_TRANSPORT &&
		    ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0))) {
			if (natt_candidate == NULL) {
				natt_candidate = sav;
				continue;
			} else {
				candidate = natt_candidate;
			}
		} else {
			if (no_natt_candidate == NULL) {
				no_natt_candidate = sav;
				continue;
			} else {
				candidate = no_natt_candidate;
			}
		}

		                              

		                    
		if (candidate->lft_c == NULL || sav->lft_c == NULL) {
			panic("key_do_allocsa_policy: "
			    "lifetime_current is NULL.\n");
		}

		                                          
		if (key_preferred_oldsa) {
			if (candidate->lft_c->sadb_lifetime_addtime >
			    sav->lft_c->sadb_lifetime_addtime) {
				if ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0) {
					natt_candidate = sav;
				} else {
					no_natt_candidate = sav;
				}
			}
			continue;
			              
		}

		                                        
		if (candidate->lft_c->sadb_lifetime_addtime <
		    sav->lft_c->sadb_lifetime_addtime) {
			d = candidate;
			if ((sah->saidx.mode == IPSEC_MODE_TUNNEL &&
			    ((sav->flags & SADB_X_EXT_NATT) != 0)) ||
			    (sah->saidx.mode == IPSEC_MODE_TRANSPORT &&
			    ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0))) {
				natt_candidate = sav;
			} else {
				no_natt_candidate = sav;
			}
		} else {
			d = sav;
		}

		  
                                                 
                                                         
               
     
		if (d->lft_c->sadb_lifetime_addtime != 0) {
			key_send_delete(d);
		}
	}

	                                         
	if (natt_candidate == NULL) {
		candidate = no_natt_candidate;
	} else if (no_natt_candidate == NULL) {
		candidate = natt_candidate;
	} else if (sah->saidx.mode == IPSEC_MODE_TUNNEL && dstport) {
		candidate = natt_candidate;
	} else if (natt_candidate->lft_c->sadb_lifetime_addtime >
	    no_natt_candidate->lft_c->sadb_lifetime_addtime) {
		candidate = natt_candidate;
	} else {
		candidate = no_natt_candidate;
	}

	if (candidate) {
		candidate->refcnt++;
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP allocsa_policy cause "
		    "refcnt++:%d SA:0x%llx\n", candidate->refcnt,
		    (uint64_t)VM_KERNEL_ADDRPERM(candidate)));
	}
	return candidate;
}

  
                                                
                                 
                                   
                                       
  
                                                                             
                              
                                                                       
                                                                        
                                            
                                                                     
                                                               
                                                                    
   
struct secasvar *
key_allocsa(
	u_int family,
	caddr_t src,
	caddr_t dst,
	u_int proto,
	u_int32_t spi)
{
	return key_allocsa_extended(family, src, dst, proto, spi, NULL);
}

struct secasvar *
key_allocsa_extended(u_int family,
    caddr_t src,
    caddr_t dst,
    u_int proto,
    u_int32_t spi,
    ifnet_t interface)
{
	struct secasvar *sav, *match;
	u_int stateidx, state, tmpidx, matchidx;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	const u_int *saorder_state_valid;
	int arraysize;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (src == NULL || dst == NULL) {
		panic("key_allocsa: NULL pointer is passed.\n");
	}

	  
                                                          
                                                           
    
	if (key_preferred_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}

	  
                  
                                                               
                                                            
                                                   
    
	  
                                                 
                                      
    
	match = NULL;
	matchidx = arraysize;
	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sav, &spihash[SPIHASH(spi)], spihash) {
		if (sav->spi != spi) {
			continue;
		}
		if (interface != NULL &&
		    sav->sah->ipsec_if != interface) {
			continue;
		}
		if (proto != sav->sah->saidx.proto) {
			continue;
		}
		if (family != sav->sah->saidx.src.ss_family ||
		    family != sav->sah->saidx.dst.ss_family) {
			continue;
		}
		tmpidx = arraysize;
		for (stateidx = 0; stateidx < matchidx; stateidx++) {
			state = saorder_state_valid[stateidx];
			if (sav->state == state) {
				tmpidx = stateidx;
				break;
			}
		}
		if (tmpidx >= matchidx) {
			continue;
		}

		                       
		switch (family) {
		case AF_INET:
			bzero(&sin, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_len = sizeof(sin);
			bcopy(dst, &sin.sin_addr,
			    sizeof(sin.sin_addr));
			if (key_sockaddrcmp((struct sockaddr*)&sin,
			    (struct sockaddr *)&sav->sah->saidx.dst, 0) != 0) {
				continue;
			}

			break;
		case AF_INET6:
			bzero(&sin6, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			sin6.sin6_len = sizeof(sin6);
			bcopy(dst, &sin6.sin6_addr,
			    sizeof(sin6.sin6_addr));
			if (IN6_IS_SCOPE_LINKLOCAL(&sin6.sin6_addr)) {
				                       
				sin6.sin6_scope_id =
				    ntohs(sin6.sin6_addr.s6_addr16[1]);
				sin6.sin6_addr.s6_addr16[1] = 0;
			}
			if (key_sockaddrcmp((struct sockaddr*)&sin6,
			    (struct sockaddr *)&sav->sah->saidx.dst, 0) != 0) {
				continue;
			}
			break;
		default:
			ipseclog((LOG_DEBUG, "key_allocsa: "
			    "unknown address family=%d.\n", family));
			continue;
		}

		match = sav;
		matchidx = tmpidx;
	}
	if (match) {
		goto found;
	}

	               
	lck_mtx_unlock(sadb_mutex);
	return NULL;

found:
	match->refcnt++;
	lck_mtx_unlock(sadb_mutex);
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP allocsa cause refcnt++:%d SA:0x%llx\n",
	    match->refcnt, (uint64_t)VM_KERNEL_ADDRPERM(match)));
	return match;
}

  
                                                                     
                                                                        
                                                                                  
   
bool
key_checksa_present(u_int family,
    caddr_t local_addr,
    caddr_t remote_addr,
    u_int16_t local_port,
    u_int16_t remote_port)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (local_addr == NULL || remote_addr == NULL) {
		panic("key_allocsa: NULL pointer is passed.\n");
	}

	  
                  
                                                               
                                                            
                                                   
    
	  
                                                 
                                      
    
	struct secashead *sah = NULL;
	bool found_sa = false;

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}

		if (sah->dir != IPSEC_DIR_OUTBOUND) {
			continue;
		}

		if (family != sah->saidx.src.ss_family) {
			continue;
		}

		struct sockaddr_in src_in = {};
		struct sockaddr_in6 src_in6 = {};

		                       
		switch (family) {
		case AF_INET:
			src_in.sin_family = AF_INET;
			src_in.sin_len = sizeof(src_in);
			memcpy(&src_in.sin_addr, local_addr, sizeof(src_in.sin_addr));
			if (key_sockaddrcmp((struct sockaddr*)&src_in,
			    (struct sockaddr *)&sah->saidx.src, 0) != 0) {
				continue;
			}
			break;
		case AF_INET6:
			src_in6.sin6_family = AF_INET6;
			src_in6.sin6_len = sizeof(src_in6);
			memcpy(&src_in6.sin6_addr, local_addr, sizeof(src_in6.sin6_addr));
			if (IN6_IS_SCOPE_LINKLOCAL(&src_in6.sin6_addr)) {
				                       
				src_in6.sin6_scope_id =
				    ntohs(src_in6.sin6_addr.s6_addr16[1]);
				src_in6.sin6_addr.s6_addr16[1] = 0;
			}
			if (key_sockaddrcmp((struct sockaddr*)&src_in6,
			    (struct sockaddr *)&sah->saidx.src, 0) != 0) {
				continue;
			}
			break;
		default:
			ipseclog((LOG_DEBUG, "key_checksa_present: "
			    "unknown address family=%d.\n",
			    family));
			continue;
		}

		struct sockaddr_in dest_in = {};
		struct sockaddr_in6 dest_in6 = {};

		                       
		switch (family) {
		case AF_INET:
			dest_in.sin_family = AF_INET;
			dest_in.sin_len = sizeof(dest_in);
			memcpy(&dest_in.sin_addr, remote_addr, sizeof(dest_in.sin_addr));
			if (key_sockaddrcmp((struct sockaddr*)&dest_in,
			    (struct sockaddr *)&sah->saidx.dst, 0) != 0) {
				continue;
			}

			break;
		case AF_INET6:
			dest_in6.sin6_family = AF_INET6;
			dest_in6.sin6_len = sizeof(dest_in6);
			memcpy(&dest_in6.sin6_addr, remote_addr, sizeof(dest_in6.sin6_addr));
			if (IN6_IS_SCOPE_LINKLOCAL(&dest_in6.sin6_addr)) {
				                       
				dest_in6.sin6_scope_id =
				    ntohs(dest_in6.sin6_addr.s6_addr16[1]);
				dest_in6.sin6_addr.s6_addr16[1] = 0;
			}
			if (key_sockaddrcmp((struct sockaddr*)&dest_in6,
			    (struct sockaddr *)&sah->saidx.dst, 0) != 0) {
				continue;
			}

			break;
		default:
			ipseclog((LOG_DEBUG, "key_checksa_present: "
			    "unknown address family=%d.\n", family));
			continue;
		}

		struct secasvar *nextsav = NULL;
		for (u_int stateidx = 0; stateidx < _ARRAYLEN(saorder_state_alive); stateidx++) {
			u_int state = saorder_state_alive[stateidx];
			for (struct secasvar *sav = LIST_FIRST(&sah->savtree[state]); sav != NULL; sav = nextsav) {
				nextsav = LIST_NEXT(sav, chain);
				                  
				if (sav->state != state) {
					ipseclog((LOG_DEBUG, "key_checksa_present: "
					    "invalid sav->state "
					    "(state: %d SA: %d)\n",
					    state, sav->state));
					continue;
				}

				if (sav->remote_ike_port != ntohs(remote_port)) {
					continue;
				}

				if (sav->natt_encapsulated_src_port != local_port) {
					continue;
				}
				found_sa = true;;
				break;
			}
		}
	}

	               
	lck_mtx_unlock(sadb_mutex);
	return found_sa;
}

u_int16_t
key_natt_get_translated_port(
	struct secasvar *outsav)
{
	struct secasindex saidx;
	struct secashead *sah;
	u_int stateidx, state;
	const u_int *saorder_state_valid;
	int arraysize;

	                         
	saidx.mode = outsav->sah->saidx.mode;
	saidx.reqid = 0;
	saidx.proto = outsav->sah->saidx.proto;
	bcopy(&outsav->sah->saidx.src, &saidx.dst, sizeof(struct sockaddr_in));
	bcopy(&outsav->sah->saidx.dst, &saidx.src, sizeof(struct sockaddr_in));

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_MODE)) {
			goto found;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return 0;

found:
	  
                                                
                                             
                                                        
    
	  
                                                  
                                   
    
	if (key_preferred_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}

	for (stateidx = 0; stateidx < arraysize; stateidx++) {
		state = saorder_state_valid[stateidx];
		if (key_do_get_translated_port(sah, outsav, state)) {
			lck_mtx_unlock(sadb_mutex);
			return outsav->natt_encapsulated_src_port;
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return 0;
}

static int
key_do_get_translated_port(
	struct secashead *sah,
	struct secasvar *outsav,
	u_int state)
{
	struct secasvar *currsav, *nextsav, *candidate;


	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	               
	candidate = NULL;

	for (currsav = LIST_FIRST(&sah->savtree[state]);
	    currsav != NULL;
	    currsav = nextsav) {
		nextsav = LIST_NEXT(currsav, chain);

		                  
		KEY_CHKSASTATE(currsav->state, state, "key_do_get_translated_port");

		if ((currsav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) == 0 ||
		    currsav->remote_ike_port != outsav->remote_ike_port) {
			continue;
		}

		if (candidate == NULL) {
			candidate = currsav;
			continue;
		}

		                              

		                    
		if (candidate->lft_c == NULL || currsav->lft_c == NULL) {
			panic("key_do_get_translated_port: "
			    "lifetime_current is NULL.\n");
		}

		                                          
		if (key_preferred_oldsa) {
			if (candidate->lft_c->sadb_lifetime_addtime >
			    currsav->lft_c->sadb_lifetime_addtime) {
				candidate = currsav;
			}
			continue;
			              
		}

		                                        
		if (candidate->lft_c->sadb_lifetime_addtime <
		    currsav->lft_c->sadb_lifetime_addtime) {
			candidate = currsav;
		}
	}

	if (candidate) {
		outsav->natt_encapsulated_src_port = candidate->natt_encapsulated_src_port;
		return 1;
	}

	return 0;
}

  
                                              
   
void
key_freesp(
	struct secpolicy *sp,
	int locked)
{
	                  
	if (sp == NULL) {
		panic("key_freesp: NULL pointer is passed.\n");
	}

	if (!locked) {
		lck_mtx_lock(sadb_mutex);
	} else {
		LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	}
	sp->refcnt--;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP freesp cause refcnt--:%d SP:0x%llx\n",
	    sp->refcnt, (uint64_t)VM_KERNEL_ADDRPERM(sp)));

	if (sp->refcnt == 0) {
		key_delsp(sp);
	}
	if (!locked) {
		lck_mtx_unlock(sadb_mutex);
	}
	return;
}

  
                                              
                                                                    
                
   
void
key_freesav(
	struct secasvar *sav,
	int locked)
{
	                  
	if (sav == NULL) {
		panic("key_freesav: NULL pointer is passed.\n");
	}

	if (!locked) {
		lck_mtx_lock(sadb_mutex);
	} else {
		LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	}
	sav->refcnt--;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP freesav cause refcnt--:%d SA:0x%llx SPI %u\n",
	    sav->refcnt, (uint64_t)VM_KERNEL_ADDRPERM(sav),
	    (u_int32_t)ntohl(sav->spi)));

	if (sav->refcnt == 0) {
		key_delsav(sav);
	}
	if (!locked) {
		lck_mtx_unlock(sadb_mutex);
	}
	return;
}

                        
  
                              
   
static void
key_delsp(
	struct secpolicy *sp)
{
	                  
	if (sp == NULL) {
		panic("key_delsp: NULL pointer is passed.\n");
	}

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	sp->state = IPSEC_SPSTATE_DEAD;

	if (sp->refcnt > 0) {
		return;                 
	}
	                          
	if (__LIST_CHAINED(sp)) {
		LIST_REMOVE(sp, chain);
		ipsec_policy_count--;
	}

	if (sp->spidx.internal_if) {
		ifnet_release(sp->spidx.internal_if);
		sp->spidx.internal_if = NULL;
	}

	if (sp->ipsec_if) {
		ifnet_release(sp->ipsec_if);
		sp->ipsec_if = NULL;
	}

	if (sp->outgoing_if) {
		ifnet_release(sp->outgoing_if);
		sp->outgoing_if = NULL;
	}

	{
		struct ipsecrequest *isr = sp->req, *nextisr;

		while (isr != NULL) {
			nextisr = isr->next;
			KFREE(isr);
			isr = nextisr;
		}
	}
	keydb_delsecpolicy(sp);

	return;
}

  
             
                        
                                   
   
static struct secpolicy *
key_getsp(
	struct secpolicyindex *spidx)
{
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (spidx == NULL) {
		panic("key_getsp: NULL pointer is passed.\n");
	}

	LIST_FOREACH(sp, &sptree[spidx->dir], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD) {
			continue;
		}
		if (key_cmpspidx_exactly(spidx, &sp->spidx)) {
			sp->refcnt++;
			return sp;
		}
	}

	return NULL;
}

  
                   
                        
                                   
   
struct secpolicy *
key_getspbyid(
	u_int32_t id)
{
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	lck_mtx_lock(sadb_mutex);
	sp = __key_getspbyid(id);
	lck_mtx_unlock(sadb_mutex);

	return sp;
}

static struct secpolicy *
__key_getspbyid(u_int32_t id)
{
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	LIST_FOREACH(sp, &sptree[IPSEC_DIR_INBOUND], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD) {
			continue;
		}
		if (sp->id == id) {
			sp->refcnt++;
			return sp;
		}
	}

	LIST_FOREACH(sp, &sptree[IPSEC_DIR_OUTBOUND], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD) {
			continue;
		}
		if (sp->id == id) {
			sp->refcnt++;
			return sp;
		}
	}

	return NULL;
}

struct secpolicy *
key_newsp(void)
{
	struct secpolicy *newsp = NULL;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);
	newsp = keydb_newsecpolicy();
	if (!newsp) {
		return newsp;
	}

	newsp->refcnt = 1;
	newsp->req = NULL;

	return newsp;
}

  
                                                           
                                                                      
                                 
   
struct secpolicy *
key_msg2sp(
	struct sadb_x_policy *xpl0,
	size_t len,
	int *error)
{
	struct secpolicy *newsp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (xpl0 == NULL) {
		panic("key_msg2sp: NULL pointer was passed.\n");
	}
	if (len < sizeof(*xpl0)) {
		panic("key_msg2sp: invalid length.\n");
	}
	if (len != PFKEY_EXTLEN(xpl0)) {
		ipseclog((LOG_DEBUG, "key_msg2sp: Invalid msg length.\n"));
		*error = EINVAL;
		return NULL;
	}

	if ((newsp = key_newsp()) == NULL) {
		*error = ENOBUFS;
		return NULL;
	}

	newsp->spidx.dir = xpl0->sadb_x_policy_dir;
	newsp->policy = xpl0->sadb_x_policy_type;

	                  
	switch (xpl0->sadb_x_policy_type) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_GENERATE:
	case IPSEC_POLICY_NONE:
	case IPSEC_POLICY_ENTRUST:
	case IPSEC_POLICY_BYPASS:
		newsp->req = NULL;
		break;

	case IPSEC_POLICY_IPSEC:
	{
		int tlen;
		struct sadb_x_ipsecrequest *xisr;
		struct ipsecrequest **p_isr = &newsp->req;

		                    
		if (PFKEY_EXTLEN(xpl0) < sizeof(*xpl0)) {
			ipseclog((LOG_DEBUG,
			    "key_msg2sp: Invalid msg length.\n"));
			key_freesp(newsp, KEY_SADB_UNLOCKED);
			*error = EINVAL;
			return NULL;
		}

		tlen = PFKEY_EXTLEN(xpl0) - sizeof(*xpl0);
		xisr = (struct sadb_x_ipsecrequest *)(xpl0 + 1);

		while (tlen > 0) {
			if (tlen < sizeof(*xisr)) {
				ipseclog((LOG_DEBUG, "key_msg2sp: "
				    "invalid ipsecrequest.\n"));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EINVAL;
				return NULL;
			}

			                  
			if (xisr->sadb_x_ipsecrequest_len < sizeof(*xisr)) {
				ipseclog((LOG_DEBUG, "key_msg2sp: "
				    "invalid ipsecrequest length.\n"));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EINVAL;
				return NULL;
			}

			                             
			KMALLOC_WAIT(*p_isr, struct ipsecrequest *, sizeof(**p_isr));
			if ((*p_isr) == NULL) {
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: No more memory.\n"));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = ENOBUFS;
				return NULL;
			}
			bzero(*p_isr, sizeof(**p_isr));

			                
			(*p_isr)->next = NULL;

			switch (xisr->sadb_x_ipsecrequest_proto) {
			case IPPROTO_ESP:
			case IPPROTO_AH:
				break;
			default:
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: invalid proto type=%u\n",
				    xisr->sadb_x_ipsecrequest_proto));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EPROTONOSUPPORT;
				return NULL;
			}
			(*p_isr)->saidx.proto = xisr->sadb_x_ipsecrequest_proto;

			switch (xisr->sadb_x_ipsecrequest_mode) {
			case IPSEC_MODE_TRANSPORT:
			case IPSEC_MODE_TUNNEL:
				break;
			case IPSEC_MODE_ANY:
			default:
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: invalid mode=%u\n",
				    xisr->sadb_x_ipsecrequest_mode));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EINVAL;
				return NULL;
			}
			(*p_isr)->saidx.mode = xisr->sadb_x_ipsecrequest_mode;

			switch (xisr->sadb_x_ipsecrequest_level) {
			case IPSEC_LEVEL_DEFAULT:
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			case IPSEC_LEVEL_UNIQUE:
				                    
				  
                                               
                                  
       
				if (xisr->sadb_x_ipsecrequest_reqid
				    > IPSEC_MANUAL_REQID_MAX) {
					ipseclog((LOG_DEBUG,
					    "key_msg2sp: reqid=%d range "
					    "violation, updated by kernel.\n",
					    xisr->sadb_x_ipsecrequest_reqid));
					xisr->sadb_x_ipsecrequest_reqid = 0;
				}

				                                             
				if (xisr->sadb_x_ipsecrequest_reqid == 0) {
					u_int16_t reqid;
					if ((reqid = key_newreqid()) == 0) {
						key_freesp(newsp, KEY_SADB_UNLOCKED);
						*error = ENOBUFS;
						return NULL;
					}
					(*p_isr)->saidx.reqid = reqid;
					xisr->sadb_x_ipsecrequest_reqid = reqid;
				} else {
					                               
					(*p_isr)->saidx.reqid =
					    xisr->sadb_x_ipsecrequest_reqid;
				}
				break;

			default:
				ipseclog((LOG_DEBUG, "key_msg2sp: invalid level=%u\n",
				    xisr->sadb_x_ipsecrequest_level));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EINVAL;
				return NULL;
			}
			(*p_isr)->level = xisr->sadb_x_ipsecrequest_level;

			                               
			if (xisr->sadb_x_ipsecrequest_len > sizeof(*xisr)) {
				struct sockaddr *paddr;

				if (tlen < xisr->sadb_x_ipsecrequest_len) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
					    "address length.\n"));
					key_freesp(newsp, KEY_SADB_UNLOCKED);
					*error = EINVAL;
					return NULL;
				}

				paddr = (struct sockaddr *)(xisr + 1);
				uint8_t src_len = paddr->sa_len;

				                                        
				if (xisr->sadb_x_ipsecrequest_len < sizeof(*xisr) + src_len + sizeof(uint8_t)) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
					    "invalid source address length.\n"));
					key_freesp(newsp, KEY_SADB_UNLOCKED);
					*error = EINVAL;
					return NULL;
				}

				                    
				if (paddr->sa_len
				    > sizeof((*p_isr)->saidx.src)) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
					    "address length.\n"));
					key_freesp(newsp, KEY_SADB_UNLOCKED);
					*error = EINVAL;
					return NULL;
				}

				bcopy(paddr, &(*p_isr)->saidx.src,
				    MIN(paddr->sa_len, sizeof((*p_isr)->saidx.src)));

				paddr = (struct sockaddr *)((caddr_t)paddr + paddr->sa_len);
				uint8_t dst_len = paddr->sa_len;

				if (xisr->sadb_x_ipsecrequest_len < sizeof(*xisr) + src_len + dst_len) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
					    "invalid dest address length.\n"));
					key_freesp(newsp, KEY_SADB_UNLOCKED);
					*error = EINVAL;
					return NULL;
				}

				                    
				if (paddr->sa_len
				    > sizeof((*p_isr)->saidx.dst)) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
					    "address length.\n"));
					key_freesp(newsp, KEY_SADB_UNLOCKED);
					*error = EINVAL;
					return NULL;
				}

				bcopy(paddr, &(*p_isr)->saidx.dst,
				    MIN(paddr->sa_len, sizeof((*p_isr)->saidx.dst)));
			}

			(*p_isr)->sp = newsp;

			                                  
			p_isr = &(*p_isr)->next;
			tlen -= xisr->sadb_x_ipsecrequest_len;

			                    
			if (tlen < 0) {
				ipseclog((LOG_DEBUG, "key_msg2sp: becoming tlen < 0.\n"));
				key_freesp(newsp, KEY_SADB_UNLOCKED);
				*error = EINVAL;
				return NULL;
			}

			xisr = (struct sadb_x_ipsecrequest *)(void *)
			    ((caddr_t)xisr + xisr->sadb_x_ipsecrequest_len);
		}
	}
	break;
	default:
		ipseclog((LOG_DEBUG, "key_msg2sp: invalid policy type.\n"));
		key_freesp(newsp, KEY_SADB_UNLOCKED);
		*error = EINVAL;
		return NULL;
	}

	*error = 0;
	return newsp;
}

static u_int16_t
key_newreqid(void)
{
	lck_mtx_lock(sadb_mutex);
	static u_int16_t auto_reqid = IPSEC_MANUAL_REQID_MAX + 1;
	int done = 0;

	                                                                                   
                                                                                  
                                                                                   
                                                                           

	while (!done) {
		struct secpolicy *sp;
		struct ipsecrequest *isr;
		int dir;

		auto_reqid = (auto_reqid == 0xFFFF
		    ? IPSEC_MANUAL_REQID_MAX + 1 : auto_reqid + 1);

		                          
		done = 1;
		for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
			LIST_FOREACH(sp, &sptree[dir], chain) {
				for (isr = sp->req; isr != NULL; isr = isr->next) {
					if (isr->saidx.reqid == auto_reqid) {
						done = 0;
						break;
					}
				}
				if (done == 0) {
					break;
				}
			}
			if (done == 0) {
				break;
			}
		}
	}

	lck_mtx_unlock(sadb_mutex);
	return auto_reqid;
}

  
                                                              
   
struct mbuf *
key_sp2msg(
	struct secpolicy *sp)
{
	struct sadb_x_policy *xpl;
	u_int tlen;
	caddr_t p;
	struct mbuf *m;

	                   
	if (sp == NULL) {
		panic("key_sp2msg: NULL pointer was passed.\n");
	}

	tlen = key_getspreqmsglen(sp);
	if (PFKEY_UNIT64(tlen) > UINT16_MAX) {
		ipseclog((LOG_ERR, "key_getspreqmsglen returned length %u\n",
		    tlen));
		return NULL;
	}

	m = key_alloc_mbuf(tlen);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	m->m_len = tlen;
	m->m_next = NULL;
	xpl = mtod(m, struct sadb_x_policy *);
	bzero(xpl, tlen);

	xpl->sadb_x_policy_len = (u_int16_t)PFKEY_UNIT64(tlen);
	xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	xpl->sadb_x_policy_type = (u_int16_t)sp->policy;
	xpl->sadb_x_policy_dir = sp->spidx.dir;
	xpl->sadb_x_policy_id = sp->id;
	p = (caddr_t)xpl + sizeof(*xpl);

	                                  
	if (sp->policy == IPSEC_POLICY_IPSEC) {
		struct sadb_x_ipsecrequest *xisr;
		struct ipsecrequest *isr;

		for (isr = sp->req; isr != NULL; isr = isr->next) {
			xisr = (struct sadb_x_ipsecrequest *)(void *)p;

			xisr->sadb_x_ipsecrequest_proto = isr->saidx.proto;
			xisr->sadb_x_ipsecrequest_mode = isr->saidx.mode;
			xisr->sadb_x_ipsecrequest_level = (u_int8_t)isr->level;
			xisr->sadb_x_ipsecrequest_reqid = (u_int16_t)isr->saidx.reqid;

			p += sizeof(*xisr);
			bcopy(&isr->saidx.src, p, isr->saidx.src.ss_len);
			p += isr->saidx.src.ss_len;
			bcopy(&isr->saidx.dst, p, isr->saidx.dst.ss_len);
			p += isr->saidx.src.ss_len;

			xisr->sadb_x_ipsecrequest_len =
			    PFKEY_ALIGN8(sizeof(*xisr)
			    + isr->saidx.src.ss_len
			    + isr->saidx.dst.ss_len);
		}
	}

	return m;
}

                                      
static struct mbuf *
key_gather_mbuf(struct mbuf *m, const struct sadb_msghdr *mhp,
    int ndeep, int nitem, int *items)
{
	int idx;
	int i;
	struct mbuf *result = NULL, *n;
	int len;

	if (m == NULL || mhp == NULL) {
		panic("null pointer passed to key_gather");
	}

	for (i = 0; i < nitem; i++) {
		idx = items[i];
		if (idx < 0 || idx > SADB_EXT_MAX) {
			goto fail;
		}
		                                           
		if (idx == SADB_EXT_RESERVED && mhp->msg == NULL) {
			continue;
		}
		if (idx != SADB_EXT_RESERVED &&
		    (mhp->ext[idx] == NULL || mhp->extlen[idx] == 0)) {
			continue;
		}

		if (idx == SADB_EXT_RESERVED) {
			len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
			MGETHDR(n, M_WAITOK, MT_DATA);                                               
			if (!n) {
				goto fail;
			}
			n->m_len = len;
			n->m_next = NULL;
			m_copydata(m, 0, sizeof(struct sadb_msg),
			    mtod(n, caddr_t));
		} else if (i < ndeep) {
			len = mhp->extlen[idx];
			n = key_alloc_mbuf(len);
			if (!n || n->m_next) {         
				if (n) {
					m_freem(n);
				}
				goto fail;
			}
			m_copydata(m, mhp->extoff[idx], mhp->extlen[idx],
			    mtod(n, caddr_t));
		} else {
			n = m_copym(m, mhp->extoff[idx], mhp->extlen[idx],
			    M_WAITOK);
		}
		if (n == NULL) {
			goto fail;
		}

		if (result) {
			m_cat(result, n);
		} else {
			result = n;
		}
	}

	if ((result->m_flags & M_PKTHDR) != 0) {
		result->m_pkthdr.len = 0;
		for (n = result; n; n = n->m_next) {
			result->m_pkthdr.len += n->m_len;
		}
	}

	return result;

fail:
	m_freem(result);
	return NULL;
}

  
                                                                 
                                            
                                               
                    
                         
           
                                               
                                
  
                                    
                                                           
                                           
  
                          
   
static int
key_spdadd(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_address *src0, *dst0, *src1 = NULL, *dst1 = NULL;
	struct sadb_x_policy *xpl0, *xpl;
	struct sadb_lifetime *lft = NULL;
	struct secpolicyindex spidx;
	struct secpolicy *newsp;
	struct timeval tv;
	ifnet_t internal_if = NULL;
	char *outgoing_if = NULL;
	char *ipsec_if = NULL;
	struct sadb_x_ipsecif *ipsecifopts = NULL;
	int error;
	int use_src_range = 0;
	int use_dst_range = 0;
	int init_disabled = 0;
	int address_family, address_len;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spdadd: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_START] != NULL && mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_END] != NULL) {
		use_src_range = 1;
	}
	if (mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_START] != NULL && mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_END] != NULL) {
		use_dst_range = 1;
	}

	if ((!use_src_range && mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL) ||
	    (!use_dst_range && mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if ((use_src_range && (mhp->extlen[SADB_X_EXT_ADDR_RANGE_SRC_START] < sizeof(struct sadb_address)
	    || mhp->extlen[SADB_X_EXT_ADDR_RANGE_SRC_END] < sizeof(struct sadb_address))) ||
	    (!use_src_range && mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address)) ||
	    (use_dst_range && (mhp->extlen[SADB_X_EXT_ADDR_RANGE_DST_START] < sizeof(struct sadb_address)
	    || mhp->extlen[SADB_X_EXT_ADDR_RANGE_DST_END] < sizeof(struct sadb_address))) ||
	    (!use_dst_range && mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_HARD]
		    < sizeof(struct sadb_lifetime)) {
			ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
			return key_senderror(so, m, EINVAL);
		}
		lft = (struct sadb_lifetime *)
		    (void *)mhp->ext[SADB_EXT_LIFETIME_HARD];
	}
	if (mhp->ext[SADB_X_EXT_IPSECIF] != NULL) {
		if (mhp->extlen[SADB_X_EXT_IPSECIF] < sizeof(struct sadb_x_ipsecif)) {
			ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
			return key_senderror(so, m, EINVAL);
		}
	}

	if (use_src_range) {
		src0 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_START];
		src1 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_END];
	} else {
		src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	}
	if (use_dst_range) {
		dst0 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_START];
		dst1 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_END];
	} else {
		dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	}
	xpl0 = (struct sadb_x_policy *)(void *)mhp->ext[SADB_X_EXT_POLICY];
	ipsecifopts = (struct sadb_x_ipsecif *)(void *)mhp->ext[SADB_X_EXT_IPSECIF];

	                     
	address_family = ((struct sockaddr *)(src0 + 1))->sa_family;
	address_len = ((struct sockaddr *)(src0 + 1))->sa_len;
	if (use_src_range) {
		if (((struct sockaddr *)(src1 + 1))->sa_family != address_family ||
		    ((struct sockaddr *)(src1 + 1))->sa_len != address_len) {
			return key_senderror(so, m, EINVAL);
		}
	}
	if (((struct sockaddr *)(dst0 + 1))->sa_family != address_family ||
	    ((struct sockaddr *)(dst0 + 1))->sa_len != address_len) {
		return key_senderror(so, m, EINVAL);
	}
	if (use_dst_range) {
		if (((struct sockaddr *)(dst1 + 1))->sa_family != address_family ||
		    ((struct sockaddr *)(dst1 + 1))->sa_len != address_len) {
			return key_senderror(so, m, EINVAL);
		}
	}

	                             
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		ipseclog((LOG_DEBUG, "key_spdadd: Invalid SP direction.\n"));
		mhp->msg->sadb_msg_errno = EINVAL;
		return 0;
	}

	                  
	                                                   
	if (xpl0->sadb_x_policy_type == IPSEC_POLICY_ENTRUST
	    || xpl0->sadb_x_policy_type == IPSEC_POLICY_BYPASS) {
		ipseclog((LOG_DEBUG, "key_spdadd: Invalid policy type.\n"));
		return key_senderror(so, m, EINVAL);
	}

	                                                         
	if (mhp->msg->sadb_msg_type != SADB_X_SPDSETIDX
	    && xpl0->sadb_x_policy_type == IPSEC_POLICY_IPSEC
	    && mhp->extlen[SADB_X_EXT_POLICY] <= sizeof(*xpl0)) {
		ipseclog((LOG_DEBUG, "key_spdadd: some policy requests part required.\n"));
		return key_senderror(so, m, EINVAL);
	}

	                        
	if (ipsecifopts != NULL) {
		if (ipsecifopts->sadb_x_ipsecif_internal_if[0]) {
			ifnet_find_by_name(ipsecifopts->sadb_x_ipsecif_internal_if, &internal_if);
		}
		if (ipsecifopts->sadb_x_ipsecif_outgoing_if[0]) {
			outgoing_if = ipsecifopts->sadb_x_ipsecif_outgoing_if;
		}
		if (ipsecifopts->sadb_x_ipsecif_ipsec_if[0]) {
			ipsec_if = ipsecifopts->sadb_x_ipsecif_ipsec_if;
		}
		init_disabled = ipsecifopts->sadb_x_ipsecif_init_disabled;
	}

	                   
	                                       
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	    src0 + 1,
	    dst0 + 1,
	    src0->sadb_address_prefixlen,
	    dst0->sadb_address_prefixlen,
	    src0->sadb_address_proto,
	    internal_if,
	    use_src_range ? src0 + 1 : NULL,
	    use_src_range ? src1 + 1 : NULL,
	    use_dst_range ? dst0 + 1 : NULL,
	    use_dst_range ? dst1 + 1 : NULL,
	    &spidx);

	  
                                        
                                                             
                                                                
               
    
	lck_mtx_lock(sadb_mutex);
	newsp = key_getsp(&spidx);
	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		if (newsp) {
			newsp->state = IPSEC_SPSTATE_DEAD;
			key_freesp(newsp, KEY_SADB_LOCKED);
		}
	} else {
		if (newsp != NULL) {
			key_freesp(newsp, KEY_SADB_LOCKED);
			ipseclog((LOG_DEBUG, "key_spdadd: a SP entry exists already.\n"));
			lck_mtx_unlock(sadb_mutex);
			if (internal_if) {
				ifnet_release(internal_if);
				internal_if = NULL;
			}
			return key_senderror(so, m, EEXIST);
		}
	}
	lck_mtx_unlock(sadb_mutex);

	                             
	if ((newsp = key_msg2sp(xpl0, PFKEY_EXTLEN(xpl0), &error)) == NULL) {
		if (internal_if) {
			ifnet_release(internal_if);
			internal_if = NULL;
		}
		return key_senderror(so, m, error);
	}

	if ((newsp->id = key_getnewspid()) == 0) {
		keydb_delsecpolicy(newsp);
		if (internal_if) {
			ifnet_release(internal_if);
			internal_if = NULL;
		}
		return key_senderror(so, m, ENOBUFS);
	}

	                                       
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	    src0 + 1,
	    dst0 + 1,
	    src0->sadb_address_prefixlen,
	    dst0->sadb_address_prefixlen,
	    src0->sadb_address_proto,
	    internal_if,
	    use_src_range ? src0 + 1 : NULL,
	    use_src_range ? src1 + 1 : NULL,
	    use_dst_range ? dst0 + 1 : NULL,
	    use_dst_range ? dst1 + 1 : NULL,
	    &newsp->spidx);

#if 1
	  
                                                              
                                                                  
    
	if (newsp->req && newsp->req->saidx.src.ss_family) {
		struct sockaddr *sa;
		sa = (struct sockaddr *)(src0 + 1);
		if (sa->sa_family != newsp->req->saidx.src.ss_family) {
			if (newsp->req->saidx.mode != IPSEC_MODE_TUNNEL || newsp->req->saidx.proto != IPPROTO_ESP) {
				keydb_delsecpolicy(newsp);
				if (internal_if) {
					ifnet_release(internal_if);
					internal_if = NULL;
				}
				return key_senderror(so, m, EINVAL);
			}
		}
	}
	if (newsp->req && newsp->req->saidx.dst.ss_family) {
		struct sockaddr *sa;
		sa = (struct sockaddr *)(dst0 + 1);
		if (sa->sa_family != newsp->req->saidx.dst.ss_family) {
			if (newsp->req->saidx.mode != IPSEC_MODE_TUNNEL || newsp->req->saidx.proto != IPPROTO_ESP) {
				keydb_delsecpolicy(newsp);
				if (internal_if) {
					ifnet_release(internal_if);
					internal_if = NULL;
				}
				return key_senderror(so, m, EINVAL);
			}
		}
	}
#endif

	microtime(&tv);
	newsp->created = tv.tv_sec;
	newsp->lastused = tv.tv_sec;
	newsp->lifetime = (long)(lft ? lft->sadb_lifetime_addtime : 0);
	newsp->validtime = (long)(lft ? lft->sadb_lifetime_usetime : 0);

	if (outgoing_if != NULL) {
		ifnet_find_by_name(outgoing_if, &newsp->outgoing_if);
	}
	if (ipsec_if != NULL) {
		ifnet_find_by_name(ipsec_if, &newsp->ipsec_if);
	}
	if (init_disabled > 0) {
		newsp->disabled = 1;
	}

	newsp->refcnt = 1;                                           
	newsp->state = IPSEC_SPSTATE_ALIVE;
	lck_mtx_lock(sadb_mutex);
	  
                                                             
                                                     
                                                 
    
	if (newsp->policy == IPSEC_POLICY_GENERATE) {
		LIST_INSERT_TAIL(&sptree[newsp->spidx.dir], newsp, secpolicy, chain);
	} else {                                                      
		struct secpolicy *tmpsp;

		LIST_FOREACH(tmpsp, &sptree[newsp->spidx.dir], chain)
		if (tmpsp->policy == IPSEC_POLICY_GENERATE) {
			break;
		}
		if (tmpsp) {
			LIST_INSERT_BEFORE(tmpsp, newsp, chain);
		} else {
			LIST_INSERT_TAIL(&sptree[newsp->spidx.dir], newsp, secpolicy, chain);
		}
		key_start_timehandler();
	}

	ipsec_policy_count++;
	                               
	if (ipsec_bypass != 0) {
		ipsec_bypass = 0;
	}

	                                   
	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		struct secspacq *spacq;
		if ((spacq = key_getspacq(&spidx)) != NULL) {
			                                                        
			microtime(&tv);
			spacq->created = tv.tv_sec;
			spacq->count = 0;
		}
	}
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n, *mpolicy;
		struct sadb_msg *newmsg;
		int off;

		                                   
		if (lft) {
			int     mbufItems[] = {SADB_EXT_RESERVED, SADB_X_EXT_POLICY,
				               SADB_EXT_LIFETIME_HARD, SADB_EXT_ADDRESS_SRC,
				               SADB_EXT_ADDRESS_DST, SADB_X_EXT_ADDR_RANGE_SRC_START, SADB_X_EXT_ADDR_RANGE_SRC_END,
				               SADB_X_EXT_ADDR_RANGE_DST_START, SADB_X_EXT_ADDR_RANGE_DST_END};
			n = key_gather_mbuf(m, mhp, 2, sizeof(mbufItems) / sizeof(int), mbufItems);
		} else {
			int     mbufItems[] = {SADB_EXT_RESERVED, SADB_X_EXT_POLICY,
				               SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST,
				               SADB_X_EXT_ADDR_RANGE_SRC_START, SADB_X_EXT_ADDR_RANGE_SRC_END,
				               SADB_X_EXT_ADDR_RANGE_DST_START, SADB_X_EXT_ADDR_RANGE_DST_END};
			n = key_gather_mbuf(m, mhp, 2, sizeof(mbufItems) / sizeof(int), mbufItems);
		}
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(*newmsg)) {
			n = m_pullup(n, sizeof(*newmsg));
			if (!n) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;

		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		off = 0;
		mpolicy = m_pulldown(n, PFKEY_ALIGN8(sizeof(struct sadb_msg)),
		    sizeof(*xpl), &off);
		if (mpolicy == NULL) {
			                        
			return key_senderror(so, m, ENOBUFS);
		}
		xpl = (struct sadb_x_policy *)(void *)(mtod(mpolicy, caddr_t) + off);
		if (xpl->sadb_x_policy_exttype != SADB_X_EXT_POLICY) {
			m_freem(n);
			return key_senderror(so, m, EINVAL);
		}
		xpl->sadb_x_policy_id = newsp->id;

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                     
       
              
                   
   
static u_int32_t
key_getnewspid(void)
{
	u_int32_t newid = 0;
	int count = key_spi_trycnt;              
	struct secpolicy *sp;

	                                            
	lck_mtx_lock(sadb_mutex);
	while (count--) {
		newid = (policy_id = (policy_id == ~0 ? 1 : policy_id + 1));

		if ((sp = __key_getspbyid(newid)) == NULL) {
			break;
		}

		key_freesp(sp, KEY_SADB_LOCKED);
	}
	lck_mtx_unlock(sadb_mutex);
	if (count == 0 || newid == 0) {
		ipseclog((LOG_DEBUG, "key_getnewspid: to allocate policy id is failed.\n"));
		return 0;
	}

	return newid;
}

  
                            
          
                                   
                                               
            
                                   
                
                                           
  
                          
   
static int
key_spddelete(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_address *src0, *dst0, *src1 = NULL, *dst1 = NULL;
	struct sadb_x_policy *xpl0;
	struct secpolicyindex spidx;
	struct secpolicy *sp;
	ifnet_t internal_if = NULL;
	struct sadb_x_ipsecif *ipsecifopts = NULL;
	int use_src_range = 0;
	int use_dst_range = 0;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spddelete: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_START] != NULL && mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_END] != NULL) {
		use_src_range = 1;
	}
	if (mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_START] != NULL && mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_END] != NULL) {
		use_dst_range = 1;
	}

	if ((!use_src_range && mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL) ||
	    (!use_dst_range && mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		ipseclog((LOG_DEBUG, "key_spddelete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if ((use_src_range && (mhp->extlen[SADB_X_EXT_ADDR_RANGE_SRC_START] < sizeof(struct sadb_address)
	    || mhp->extlen[SADB_X_EXT_ADDR_RANGE_SRC_END] < sizeof(struct sadb_address))) ||
	    (!use_src_range && mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address)) ||
	    (use_dst_range && (mhp->extlen[SADB_X_EXT_ADDR_RANGE_DST_START] < sizeof(struct sadb_address)
	    || mhp->extlen[SADB_X_EXT_ADDR_RANGE_DST_END] < sizeof(struct sadb_address))) ||
	    (!use_dst_range && mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spddelete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (use_src_range) {
		src0 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_START];
		src1 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_SRC_END];
	} else {
		src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	}
	if (use_dst_range) {
		dst0 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_START];
		dst1 = (struct sadb_address *)mhp->ext[SADB_X_EXT_ADDR_RANGE_DST_END];
	} else {
		dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	}
	xpl0 = (struct sadb_x_policy *)(void *)mhp->ext[SADB_X_EXT_POLICY];
	ipsecifopts = (struct sadb_x_ipsecif *)(void *)mhp->ext[SADB_X_EXT_IPSECIF];

	                             
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		ipseclog((LOG_DEBUG, "key_spddelete: Invalid SP direction.\n"));
		return key_senderror(so, m, EINVAL);
	}

	                        
	if (ipsecifopts != NULL) {
		if (ipsecifopts->sadb_x_ipsecif_internal_if[0]) {
			ifnet_find_by_name(ipsecifopts->sadb_x_ipsecif_internal_if, &internal_if);
		}
	}

	                   
	                                       
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	    src0 + 1,
	    dst0 + 1,
	    src0->sadb_address_prefixlen,
	    dst0->sadb_address_prefixlen,
	    src0->sadb_address_proto,
	    internal_if,
	    use_src_range ? src0 + 1 : NULL,
	    use_src_range ? src1 + 1 : NULL,
	    use_dst_range ? dst0 + 1 : NULL,
	    use_dst_range ? dst1 + 1 : NULL,
	    &spidx);

	                          
	lck_mtx_lock(sadb_mutex);
	if ((sp = key_getsp(&spidx)) == NULL) {
		ipseclog((LOG_DEBUG, "key_spddelete: no SP found.\n"));
		lck_mtx_unlock(sadb_mutex);
		if (internal_if) {
			ifnet_release(internal_if);
			internal_if = NULL;
		}
		return key_senderror(so, m, EINVAL);
	}

	if (internal_if) {
		ifnet_release(internal_if);
		internal_if = NULL;
	}

	                                              
	xpl0->sadb_x_policy_id = sp->id;

	sp->state = IPSEC_SPSTATE_DEAD;
	key_freesp(sp, KEY_SADB_LOCKED);
	lck_mtx_unlock(sadb_mutex);


	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int     mbufItems[] = {SADB_EXT_RESERVED, SADB_X_EXT_POLICY,
			               SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST,
			               SADB_X_EXT_ADDR_RANGE_SRC_START, SADB_X_EXT_ADDR_RANGE_SRC_END,
			               SADB_X_EXT_ADDR_RANGE_DST_START, SADB_X_EXT_ADDR_RANGE_DST_END};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                             
          
                      
                                               
            
                      
                
                                           
  
                          
   
static int
key_spddelete2(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spddelete2: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spddelete2: invalid message is passed.\n"));
		key_senderror(so, m, EINVAL);
		return 0;
	}

	id = ((struct sadb_x_policy *)
	    (void *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	                          
	lck_mtx_lock(sadb_mutex);
	if ((sp = __key_getspbyid(id)) == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_spddelete2: no SP found id:%u.\n", id));
		return key_senderror(so, m, EINVAL);
	}

	sp->state = IPSEC_SPSTATE_DEAD;
	key_freesp(sp, KEY_SADB_LOCKED);
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n, *nn;
		struct sadb_msg *newmsg;
		int off, len;

		                                   
		len = PFKEY_ALIGN8(sizeof(struct sadb_msg));

		if (len > MCLBYTES) {
			return key_senderror(so, m, ENOBUFS);
		}
		MGETHDR(n, M_WAITOK, MT_DATA);
		if (n && len > MHLEN) {
			MCLGET(n, M_WAITOK);
			if ((n->m_flags & M_EXT) == 0) {
				m_freem(n);
				n = NULL;
			}
		}
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		n->m_len = len;
		n->m_next = NULL;
		off = 0;

		m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, caddr_t) + off);
		off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

#if DIAGNOSTIC
		if (off != len) {
			panic("length inconsistency in key_spddelete2");
		}
#endif

		n->m_next = m_copym(m, mhp->extoff[SADB_X_EXT_POLICY],
		    mhp->extlen[SADB_X_EXT_POLICY], M_WAITOK);
		if (!n->m_next) {
			m_freem(n);
			return key_senderror(so, m, ENOBUFS);
		}

		n->m_pkthdr.len = 0;
		for (nn = n; nn; nn = nn->m_next) {
			n->m_pkthdr.len += nn->m_len;
		}

		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

static int
key_spdenable(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spdenable: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spdenable: invalid message is passed.\n"));
		key_senderror(so, m, EINVAL);
		return 0;
	}

	id = ((struct sadb_x_policy *)
	    (void *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	                          
	lck_mtx_lock(sadb_mutex);
	if ((sp = __key_getspbyid(id)) == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_spdenable: no SP found id:%u.\n", id));
		return key_senderror(so, m, EINVAL);
	}

	sp->disabled = 0;
	key_freesp(sp, KEY_SADB_LOCKED);
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int mbufItems[] = {SADB_EXT_RESERVED, SADB_X_EXT_POLICY};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

static int
key_spddisable(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spddisable: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spddisable: invalid message is passed.\n"));
		key_senderror(so, m, EINVAL);
		return 0;
	}

	id = ((struct sadb_x_policy *)
	    (void *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	                          
	lck_mtx_lock(sadb_mutex);
	if ((sp = __key_getspbyid(id)) == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_spddisable: no SP found id:%u.\n", id));
		return key_senderror(so, m, EINVAL);
	}

	sp->disabled = 1;
	key_freesp(sp, KEY_SADB_LOCKED);
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int mbufItems[] = {SADB_EXT_RESERVED, SADB_X_EXT_POLICY};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                        
          
                      
                    
            
                                
                
                                           
  
                          
   
static int
key_spdget(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;
	struct mbuf *n;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spdget: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spdget: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	id = ((struct sadb_x_policy *)
	    (void *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	                          
	lck_mtx_lock(sadb_mutex);
	if ((sp = __key_getspbyid(id)) == NULL) {
		ipseclog((LOG_DEBUG, "key_spdget: no SP found id:%u.\n", id));
		lck_mtx_unlock(sadb_mutex);
		return key_senderror(so, m, ENOENT);
	}
	lck_mtx_unlock(sadb_mutex);
	n = key_setdumpsp(sp, SADB_X_SPDGET, 0, mhp->msg->sadb_msg_pid);
	key_freesp(sp, KEY_SADB_UNLOCKED);
	if (n != NULL) {
		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
	} else {
		return key_senderror(so, m, ENOBUFS);
	}
}

  
                                
                                                    
       
                      
                                
                                                     
     
                   
                                            
                                        
  
                     
                          
   
int
key_spdacquire(
	struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
	struct secspacq *newspacq;
	int error;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (sp == NULL) {
		panic("key_spdacquire: NULL pointer is passed.\n");
	}
	if (sp->req != NULL) {
		panic("key_spdacquire: called but there is request.\n");
	}
	if (sp->policy != IPSEC_POLICY_IPSEC) {
		panic("key_spdacquire: policy mismathed. IPsec is expected.\n");
	}

	                                                       
	lck_mtx_lock(sadb_mutex);
	sp->refcnt++;
	if ((newspacq = key_getspacq(&sp->spidx)) != NULL) {
		key_freesp(sp, KEY_SADB_LOCKED);
		if (key_blockacq_count < newspacq->count) {
			                                        
			newspacq->count = 0;
		} else {
			                                       
			newspacq->count++;
			lck_mtx_unlock(sadb_mutex);
			return 0;
		}
	} else {
		                                                       
		if ((newspacq = key_newspacq(&sp->spidx)) == NULL) {
			key_freesp(sp, KEY_SADB_LOCKED);
			lck_mtx_unlock(sadb_mutex);
			return ENOBUFS;
		}
		key_freesp(sp, KEY_SADB_LOCKED);
		                    
		LIST_INSERT_HEAD(&spacqtree, newspacq, chain);
		key_start_timehandler();
	}
	lck_mtx_unlock(sadb_mutex);
	                                   
	m = key_setsadbmsg(SADB_X_SPDACQUIRE, 0, 0, 0, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	VERIFY(PFKEY_UNIT64(result->m_pkthdr.len) <= UINT16_MAX);
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, m, KEY_SENDUP_REGISTERED);

fail:
	if (result) {
		m_freem(result);
	}
	return error;
}

  
                           
          
           
                                                    
            
           
               
                                                      
  
                          
   
static int
key_spdflush(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secpolicy *sp;
	u_int dir;

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spdflush: NULL pointer is passed.\n");
	}

	if (m->m_len != PFKEY_ALIGN8(sizeof(struct sadb_msg))) {
		return key_senderror(so, m, EINVAL);
	}

	lck_mtx_lock(sadb_mutex);
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			sp->state = IPSEC_SPSTATE_DEAD;
		}
	}
	lck_mtx_unlock(sadb_mutex);

	if (sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		ipseclog((LOG_DEBUG, "key_spdflush: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	if (m->m_next) {
		m_freem(m->m_next);
	}
	m->m_next = NULL;
	m->m_pkthdr.len = m->m_len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
	newmsg = mtod(m, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(m->m_pkthdr.len);

	return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
}

  
                          
          
           
                                        
            
                 
                
  
                          
   

static int
key_spddump(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct secpolicy *sp, **spbuf = NULL, **sp_ptr;
	u_int32_t cnt = 0, bufcount = 0;
	size_t total_req_size = 0;
	u_int dir;
	struct mbuf *n;
	int error = 0;

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_spddump: NULL pointer is passed.\n");
	}

	if ((bufcount = ipsec_policy_count) == 0) {
		error = ENOENT;
		goto end;
	}

	if (os_add_overflow(bufcount, 256, &bufcount)) {
		ipseclog((LOG_DEBUG, "key_spddump: bufcount overflow, ipsec policy count %u.\n", ipsec_policy_count));
		bufcount = ipsec_policy_count;
	}

	if (os_mul_overflow(bufcount, sizeof(struct secpolicy *), &total_req_size)) {
		panic("key_spddump spbuf requested memory overflow %u\n", bufcount);
	}

	KMALLOC_WAIT(spbuf, struct secpolicy**, total_req_size);
	if (spbuf == NULL) {
		ipseclog((LOG_DEBUG, "key_spddump: No more memory.\n"));
		error = ENOMEM;
		goto end;
	}
	lck_mtx_lock(sadb_mutex);
	                                  
	sp_ptr = spbuf;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			if (cnt == bufcount) {
				break;                           
			}
			*sp_ptr++ = sp;
			sp->refcnt++;
			cnt++;
		}
	}
	lck_mtx_unlock(sadb_mutex);

	if (cnt == 0) {
		error = ENOENT;
		goto end;
	}

	sp_ptr = spbuf;
	while (cnt) {
		--cnt;
		n = key_setdumpsp(*sp_ptr++, SADB_X_SPDDUMP, cnt,
		    mhp->msg->sadb_msg_pid);

		if (n) {
			key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
		}
	}

	lck_mtx_lock(sadb_mutex);
	while (sp_ptr > spbuf) {
		key_freesp(*(--sp_ptr), KEY_SADB_LOCKED);
	}
	lck_mtx_unlock(sadb_mutex);

end:
	if (spbuf) {
		KFREE(spbuf);
	}
	if (error) {
		return key_senderror(so, m, error);
	}

	m_freem(m);
	return 0;
}

static struct mbuf *
key_setdumpsp(
	struct secpolicy *sp,
	u_int8_t msg_type,
	u_int32_t seq,
	u_int32_t pid)
{
	struct mbuf *result = NULL, *m;

	m = key_setsadbmsg(msg_type, 0, SADB_SATYPE_UNSPEC, seq, pid, (u_int16_t)sp->refcnt);
	if (!m) {
		goto fail;
	}
	result = m;

	if (sp->spidx.src_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_START,
		    (struct sockaddr *)&sp->spidx.src_range.start, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_END,
		    (struct sockaddr *)&sp->spidx.src_range.end, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
		    (struct sockaddr *)&sp->spidx.src, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	if (sp->spidx.dst_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_START,
		    (struct sockaddr *)&sp->spidx.dst_range.start, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_END,
		    (struct sockaddr *)&sp->spidx.dst_range.end, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
		    (struct sockaddr *)&sp->spidx.dst, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	if (sp->spidx.internal_if || sp->outgoing_if || sp->ipsec_if || sp->disabled) {
		m = key_setsadbipsecif(sp->spidx.internal_if, sp->outgoing_if, sp->ipsec_if, sp->disabled);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	m = key_sp2msg(sp);
	if (!m) {
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	if (PFKEY_UNIT64(result->m_pkthdr.len) >= UINT16_MAX) {
		ipseclog((LOG_DEBUG, "key_setdumpsp: packet header length > UINT16_MAX\n"));
		goto fail;
	}

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return result;

fail:
	m_freem(result);
	return NULL;
}

  
                                                            
   
static u_int
key_getspreqmsglen(
	struct secpolicy *sp)
{
	u_int tlen;

	tlen = sizeof(struct sadb_x_policy);

	                                  
	if (sp->policy != IPSEC_POLICY_IPSEC) {
		return tlen;
	}

	                                  
	{
		struct ipsecrequest *isr;
		int len;

		for (isr = sp->req; isr != NULL; isr = isr->next) {
			len = sizeof(struct sadb_x_ipsecrequest)
			    + isr->saidx.src.ss_len
			    + isr->saidx.dst.ss_len;

			tlen += PFKEY_ALIGN8(len);
		}
	}

	return tlen;
}

  
                            
       
                                              
                    
  
                   
                        
   
static int
key_spdexpire(
	struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
	int len;
	int error = EINVAL;
	struct sadb_lifetime *lt;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (sp == NULL) {
		panic("key_spdexpire: NULL pointer is passed.\n");
	}

	                    
	m = key_setsadbmsg(SADB_X_SPDEXPIRE, 0, 0, 0, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	                                                  
	len = PFKEY_ALIGN8(sizeof(*lt)) * 2;
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		error = ENOBUFS;
		goto fail;
	}
	bzero(mtod(m, caddr_t), len);
	lt = mtod(m, struct sadb_lifetime *);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lt->sadb_lifetime_allocations = 0;
	lt->sadb_lifetime_bytes = 0;
	lt->sadb_lifetime_addtime = sp->created;
	lt->sadb_lifetime_usetime = sp->lastused;
	lt = (struct sadb_lifetime *)(void *)(mtod(m, caddr_t) + len / 2);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
	lt->sadb_lifetime_allocations = 0;
	lt->sadb_lifetime_bytes = 0;
	lt->sadb_lifetime_addtime = sp->lifetime;
	lt->sadb_lifetime_usetime = sp->validtime;
	m_cat(result, m);

	                                     
	if (sp->spidx.src_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_START,
		    (struct sockaddr *)&sp->spidx.src_range.start, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_END,
		    (struct sockaddr *)&sp->spidx.src_range.end, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
		    (struct sockaddr *)&sp->spidx.src, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	}

	                                   
	if (sp->spidx.dst_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_START,
		    (struct sockaddr *)&sp->spidx.dst_range.start, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_END,
		    (struct sockaddr *)&sp->spidx.dst_range.end, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
		    (struct sockaddr *)&sp->spidx.dst, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	}

	                   
	m = key_sp2msg(sp);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	if (PFKEY_UNIT64(result->m_pkthdr.len) >= UINT16_MAX) {
		ipseclog((LOG_DEBUG, "key_setdumpsp: packet header length > UINT16_MAX\n"));
		goto fail;
	}

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

fail:
	if (result) {
		m_freem(result);
	}
	return error;
}

                        
  
                                                                        
                                                 
                                   
   
static struct secashead *
key_newsah(struct secasindex *saidx,
    ifnet_t ipsec_if,
    u_int outgoing_if,
    u_int8_t dir,
    u_int16_t flags)
{
	struct secashead *newsah;

	                  
	if (saidx == NULL) {
		panic("key_newsaidx: NULL pointer is passed.\n");
	}

	VERIFY(flags == SECURITY_ASSOCIATION_PFKEY || flags == SECURITY_ASSOCIATION_CUSTOM_IPSEC);

	newsah = keydb_newsecashead();
	if (newsah == NULL) {
		return NULL;
	}

	bcopy(saidx, &newsah->saidx, sizeof(newsah->saidx));

	                      
	switch (saidx->src.ss_family) {
	case AF_INET:
		((struct sockaddr_in *)(&newsah->saidx.src))->sin_port = IPSEC_PORT_ANY;
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)(&newsah->saidx.src))->sin6_port = IPSEC_PORT_ANY;
		break;
	default:
		break;
	}
	switch (saidx->dst.ss_family) {
	case AF_INET:
		((struct sockaddr_in *)(&newsah->saidx.dst))->sin_port = IPSEC_PORT_ANY;
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)(&newsah->saidx.dst))->sin6_port = IPSEC_PORT_ANY;
		break;
	default:
		break;
	}

	newsah->outgoing_if = outgoing_if;
	if (ipsec_if) {
		ifnet_reference(ipsec_if);
		newsah->ipsec_if = ipsec_if;
	}
	newsah->dir = dir;
	                      
	newsah->state = SADB_SASTATE_MATURE;
	newsah->flags = flags;

	if (flags == SECURITY_ASSOCIATION_PFKEY) {
		LIST_INSERT_HEAD(&sahtree, newsah, chain);
	} else {
		LIST_INSERT_HEAD(&custom_sahtree, newsah, chain);
	}
	key_start_timehandler();

	return newsah;
}

  
                                        
   
void
key_delsah(
	struct secashead *sah)
{
	struct secasvar *sav, *nextsav;
	u_int stateidx, state;
	int zombie = 0;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (sah == NULL) {
		panic("key_delsah: NULL pointer is passed.\n");
	}

	if (sah->use_count > 0) {
		return;
	}

	                                                 
	for (stateidx = 0;
	    stateidx < _ARRAYLEN(saorder_state_any);
	    stateidx++) {
		state = saorder_state_any[stateidx];
		for (sav = (struct secasvar *)LIST_FIRST(&sah->savtree[state]);
		    sav != NULL;
		    sav = nextsav) {
			nextsav = LIST_NEXT(sav, chain);

			if (sav->refcnt > 0) {
				                               
				zombie++;
				continue;
			}

			                  
			KEY_CHKSASTATE(state, sav->state, "key_delsah");

			key_freesav(sav, KEY_SADB_LOCKED);

			                         
			sav->sah = NULL;
			sav = NULL;
		}
	}

	                                              
	if (zombie) {
		return;
	}

	ROUTE_RELEASE(&sah->sa_route);

	if (sah->ipsec_if) {
		ifnet_release(sah->ipsec_if);
		sah->ipsec_if = NULL;
	}

	                                  
	if (__LIST_CHAINED(sah)) {
		LIST_REMOVE(sah, chain);
	}

	KFREE(sah);

	return;
}

  
                                                                           
                                              
                                   
                                         
                      
                             
                   
                                    
  
                                                      
   
static struct secasvar *
key_newsav(
	struct mbuf *m,
	const struct sadb_msghdr *mhp,
	struct secashead *sah,
	int *errp,
	struct socket *so)
{
	struct secasvar *newsav;
	const struct sadb_sa *xsa;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (m == NULL || mhp == NULL || mhp->msg == NULL || sah == NULL) {
		panic("key_newsa: NULL pointer is passed.\n");
	}

	KMALLOC_NOWAIT(newsav, struct secasvar *, sizeof(struct secasvar));
	if (newsav == NULL) {
		lck_mtx_unlock(sadb_mutex);
		KMALLOC_WAIT(newsav, struct secasvar *, sizeof(struct secasvar));
		lck_mtx_lock(sadb_mutex);
		if (newsav == NULL) {
			ipseclog((LOG_DEBUG, "key_newsa: No more memory.\n"));
			*errp = ENOBUFS;
			return NULL;
		}
	}
	bzero((caddr_t)newsav, sizeof(struct secasvar));

	switch (mhp->msg->sadb_msg_type) {
	case SADB_GETSPI:
		key_setspi(newsav, 0);
		newsav->seq = mhp->msg->sadb_msg_seq;
		break;

	case SADB_ADD:
		                  
		if (mhp->ext[SADB_EXT_SA] == NULL) {
			key_delsav(newsav);
			ipseclog((LOG_DEBUG, "key_newsa: invalid message is passed.\n"));
			*errp = EINVAL;
			return NULL;
		}
		xsa = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
		key_setspi(newsav, xsa->sadb_sa_spi);
		newsav->seq = mhp->msg->sadb_msg_seq;
		break;
	default:
		key_delsav(newsav);
		*errp = EINVAL;
		return NULL;
	}

	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		if (((struct sadb_x_sa2 *)(void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_alwaysexpire) {
			newsav->always_expire = 1;
		}
		newsav->flags2 = ((struct sadb_x_sa2 *)(void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_flags;
		if (newsav->flags2 & SADB_X_EXT_SA2_DELETE_ON_DETACH) {
			newsav->so = so;
		}
	}

	                     
	if (mhp->msg->sadb_msg_type != SADB_GETSPI) {
		*errp = key_setsaval(newsav, m, mhp);
		if (*errp) {
			key_delsav(newsav);
			return NULL;
		}
	} else {
		                                                
		const struct sadb_lifetime *lft0;
		struct timeval tv;

		lft0 = (struct sadb_lifetime *)(void *)mhp->ext[SADB_EXT_LIFETIME_HARD];
		if (lft0 != NULL) {
			                               
			KMALLOC_NOWAIT(newsav->lft_c, struct sadb_lifetime *,
			    sizeof(struct sadb_lifetime));
			if (newsav->lft_c == NULL) {
				lck_mtx_unlock(sadb_mutex);
				KMALLOC_WAIT(newsav->lft_c, struct sadb_lifetime *,
				    sizeof(struct sadb_lifetime));
				lck_mtx_lock(sadb_mutex);
				if (newsav->lft_c == NULL) {
					ipseclog((LOG_DEBUG, "key_newsa: No more memory.\n"));
					key_delsav(newsav);
					*errp = ENOBUFS;
					return NULL;
				}
			}

			microtime(&tv);

			newsav->lft_c->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
			newsav->lft_c->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
			newsav->lft_c->sadb_lifetime_allocations = 0;
			newsav->lft_c->sadb_lifetime_bytes = 0;
			newsav->lft_c->sadb_lifetime_addtime = tv.tv_sec;
			newsav->lft_c->sadb_lifetime_usetime = 0;

			if (mhp->extlen[SADB_EXT_LIFETIME_HARD] < sizeof(*lft0)) {
				ipseclog((LOG_DEBUG, "key_newsa: invalid hard lifetime ext len.\n"));
				key_delsav(newsav);
				*errp = EINVAL;
				return NULL;
			}
			newsav->lft_h = (struct sadb_lifetime *)key_newbuf(lft0, sizeof(*lft0));
			if (newsav->lft_h == NULL) {
				ipseclog((LOG_DEBUG, "key_newsa: No more memory.\n"));
				key_delsav(newsav);
				*errp = ENOBUFS;
				return NULL;
			}
		}
	}

	                   
	{
		struct timeval tv;
		microtime(&tv);
		newsav->created = tv.tv_sec;
	}

	newsav->pid = mhp->msg->sadb_msg_pid;

	                   
	newsav->sah = sah;
	newsav->refcnt = 1;
	newsav->state = SADB_SASTATE_LARVAL;
	LIST_INSERT_TAIL(&sah->savtree[SADB_SASTATE_LARVAL], newsav,
	    secasvar, chain);
	ipsec_sav_count++;
	ipsec_monitor_sleep_wake();

	return newsav;
}

static int
key_migratesav(struct secasvar *sav,
    struct secashead *newsah)
{
	if (sav == NULL || newsah == NULL || sav->state != SADB_SASTATE_MATURE) {
		return EINVAL;
	}

	                           
	if (__LIST_CHAINED(sav)) {
		LIST_REMOVE(sav, chain);
	}

	sav->sah = newsah;
	LIST_INSERT_TAIL(&newsah->savtree[SADB_SASTATE_MATURE], sav, secasvar, chain);
	return 0;
}

static void
key_reset_sav(struct secasvar *sav)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (sav == NULL) {
		panic("key_delsav: NULL pointer is passed.\n");
	}

	sav->remote_ike_port = 0;
	sav->natt_encapsulated_src_port = 0;

	if (sav->key_auth != NULL) {
		bzero(_KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		KFREE(sav->key_auth);
		sav->key_auth = NULL;
	}
	if (sav->key_enc != NULL) {
		bzero(_KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc));
		KFREE(sav->key_enc);
		sav->key_enc = NULL;
	}
	if (sav->sched) {
		bzero(sav->sched, sav->schedlen);
		KFREE(sav->sched);
		sav->sched = NULL;
		sav->schedlen = 0;
	}

	for (int i = 0; i < MAX_REPLAY_WINDOWS; i++) {
		if (sav->replay[i] != NULL) {
			keydb_delsecreplay(sav->replay[i]);
			sav->replay[i] = NULL;
		}
	}
	if (sav->lft_c != NULL) {
		KFREE(sav->lft_c);
		sav->lft_c = NULL;
	}
	if (sav->lft_h != NULL) {
		KFREE(sav->lft_h);
		sav->lft_h = NULL;
	}
	if (sav->lft_s != NULL) {
		KFREE(sav->lft_s);
		sav->lft_s = NULL;
	}
	if (sav->iv != NULL) {
		KFREE(sav->iv);
		sav->iv = NULL;
	}

	return;
}

  
                            
   
void
key_delsav(
	struct secasvar *sav)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (sav == NULL) {
		panic("key_delsav: NULL pointer is passed.\n");
	}

	if (sav->refcnt > 0) {
		return;                         
	}
	                           
	if (__LIST_CHAINED(sav)) {
		LIST_REMOVE(sav, chain);
		ipsec_sav_count--;
	}

	if (sav->spihash.le_prev || sav->spihash.le_next) {
		LIST_REMOVE(sav, spihash);
	}

	key_reset_sav(sav);

	KFREE(sav);

	return;
}

  
              
       
                   
                                   
   
static struct secashead *
key_getsah(struct secasindex *saidx, u_int16_t flags)
{
	struct secashead *sah;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	if ((flags & SECURITY_ASSOCIATION_ANY) == SECURITY_ASSOCIATION_ANY ||
	    (flags & SECURITY_ASSOCIATION_PFKEY) == SECURITY_ASSOCIATION_PFKEY) {
		LIST_FOREACH(sah, &sahtree, chain) {
			if (sah->state == SADB_SASTATE_DEAD) {
				continue;
			}
			if (key_cmpsaidx(&sah->saidx, saidx, CMP_REQID)) {
				return sah;
			}
		}
	}

	if ((flags & SECURITY_ASSOCIATION_ANY) == SECURITY_ASSOCIATION_ANY ||
	    (flags & SECURITY_ASSOCIATION_PFKEY) == SECURITY_ASSOCIATION_CUSTOM_IPSEC) {
		LIST_FOREACH(sah, &custom_sahtree, chain) {
			if (sah->state == SADB_SASTATE_DEAD) {
				continue;
			}
			if (key_cmpsaidx(&sah->saidx, saidx, 0)) {
				return sah;
			}
		}
	}

	return NULL;
}

struct secashead *
key_newsah2(struct secasindex *saidx,
    u_int8_t           dir)
{
	struct secashead *sah;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	sah = key_getsah(saidx, SECURITY_ASSOCIATION_ANY);
	if (!sah) {
		return key_newsah(saidx, NULL, 0, dir, SECURITY_ASSOCIATION_PFKEY);
	}
	return sah;
}

  
                                  
                                                            
       
                   
                                   
   
static struct secasvar *
key_checkspidup(
	struct secasindex *saidx,
	u_int32_t spi)
{
	struct secasvar *sav;
	u_int stateidx, state;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                          
	if (saidx->src.ss_family != saidx->dst.ss_family) {
		ipseclog((LOG_DEBUG, "key_checkspidup: address family mismatched.\n"));
		return NULL;
	}

	                   
	LIST_FOREACH(sav, &spihash[SPIHASH(spi)], spihash) {
		if (sav->spi != spi) {
			continue;
		}
		for (stateidx = 0;
		    stateidx < _ARRAYLEN(saorder_state_alive);
		    stateidx++) {
			state = saorder_state_alive[stateidx];
			if (sav->state == state &&
			    key_ismyaddr((struct sockaddr *)&sav->sah->saidx.dst)) {
				return sav;
			}
		}
	}

	return NULL;
}

static void
key_setspi(
	struct secasvar *sav,
	u_int32_t spi)
{
	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	sav->spi = spi;
	if (sav->spihash.le_prev || sav->spihash.le_next) {
		LIST_REMOVE(sav, spihash);
	}
	LIST_INSERT_HEAD(&spihash[SPIHASH(spi)], sav, spihash);
}


  
                                               
       
                   
                                   
   
static struct secasvar *
key_getsavbyspi(
	struct secashead *sah,
	u_int32_t spi)
{
	struct secasvar *sav, *match;
	u_int stateidx, state, matchidx;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	match = NULL;
	matchidx = _ARRAYLEN(saorder_state_alive);
	LIST_FOREACH(sav, &spihash[SPIHASH(spi)], spihash) {
		if (sav->spi != spi) {
			continue;
		}
		if (sav->sah != sah) {
			continue;
		}
		for (stateidx = 0; stateidx < matchidx; stateidx++) {
			state = saorder_state_alive[stateidx];
			if (sav->state == state) {
				match = sav;
				matchidx = stateidx;
				break;
			}
		}
	}

	return match;
}

  
                                                                             
                                 
                   
               
  
                                                      
   
static int
key_setsaval(
	struct secasvar *sav,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
#if IPSEC_ESP
	const struct esp_algorithm *algo;
#endif
	int error = 0;
	struct timeval tv;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                  
	if (m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_setsaval: NULL pointer is passed.\n");
	}

	                    
	key_reset_sav(sav);
	sav->natt_last_activity = natt_now;

	        
	if (mhp->ext[SADB_EXT_SA] != NULL) {
		const struct sadb_sa *sa0;

		sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
		if (mhp->extlen[SADB_EXT_SA] < sizeof(*sa0)) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid message size.\n"));
			error = EINVAL;
			goto fail;
		}

		sav->alg_auth = sa0->sadb_sa_auth;
		sav->alg_enc = sa0->sadb_sa_encrypt;
		sav->flags = sa0->sadb_sa_flags;

		  
                                                      
                                   
     
		if ((sav->flags & SADB_X_EXT_NATT) != 0) {
			if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa_2) ||
			    ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_port == 0) {
				ipseclog((LOG_DEBUG, "key_setsaval: natt port not set.\n"));
				error = EINVAL;
				goto fail;
			}
			sav->natt_encapsulated_src_port = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_src_port;
			sav->remote_ike_port = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_port;
			sav->natt_interval = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_interval;
			sav->natt_offload_interval = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_offload_interval;
		}

		  
                                                             
                                                                
                                                     
     
		if ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0) {
			if ((sav->flags & SADB_X_EXT_NATT) == 0 ||
			    (sav->flags & SADB_X_EXT_NATT_KEEPALIVE) != 0) {
				sav->flags &= ~SADB_X_EXT_NATT_MULTIPLEUSERS;
			}
		}

		                   
		if ((sa0->sadb_sa_flags & SADB_X_EXT_OLD) == 0) {
			if ((sav->flags2 & SADB_X_EXT_SA2_SEQ_PER_TRAFFIC_CLASS) ==
			    SADB_X_EXT_SA2_SEQ_PER_TRAFFIC_CLASS) {
				uint32_t range = (1ULL << (sizeof(((struct secreplay *)0)->count) * 8)) / MAX_REPLAY_WINDOWS;
				for (int i = 0; i < MAX_REPLAY_WINDOWS; i++) {
					sav->replay[i] = keydb_newsecreplay(sa0->sadb_sa_replay);
					if (sav->replay[i] == NULL) {
						ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
						error = ENOBUFS;
						goto fail;
					}
					                                                  
					sav->replay[i]->count = i * range;
					sav->replay[i]->lastseq = ((i + 1) * range) - 1;
				}
			} else {
				sav->replay[0] = keydb_newsecreplay(sa0->sadb_sa_replay);
				if (sav->replay[0] == NULL) {
					ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
					error = ENOBUFS;
					goto fail;
				}
				sav->replay[0]->lastseq = ~0;
			}
		}
	}

	                         
	if (mhp->ext[SADB_EXT_KEY_AUTH] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = (const struct sadb_key *)mhp->ext[SADB_EXT_KEY_AUTH];
		len = mhp->extlen[SADB_EXT_KEY_AUTH];

		error = 0;
		if (len < sizeof(*key0)) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid auth key ext len. len = %d\n", len));
			error = EINVAL;
			goto fail;
		}
		switch (mhp->msg->sadb_msg_satype) {
		case SADB_SATYPE_AH:
		case SADB_SATYPE_ESP:
			if (len == PFKEY_ALIGN8(sizeof(struct sadb_key)) &&
			    sav->alg_auth != SADB_X_AALG_NULL) {
				error = EINVAL;
			}
			break;
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid key_auth values.\n"));
			goto fail;
		}

		sav->key_auth = (struct sadb_key *)key_newbuf(key0, len);
		if (sav->key_auth == NULL) {
			ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
	}

	                    
	if (mhp->ext[SADB_EXT_KEY_ENCRYPT] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = (const struct sadb_key *)mhp->ext[SADB_EXT_KEY_ENCRYPT];
		len = mhp->extlen[SADB_EXT_KEY_ENCRYPT];

		error = 0;
		if (len < sizeof(*key0)) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid encryption key ext len. len = %d\n", len));
			error = EINVAL;
			goto fail;
		}
		switch (mhp->msg->sadb_msg_satype) {
		case SADB_SATYPE_ESP:
			if (len == PFKEY_ALIGN8(sizeof(struct sadb_key)) &&
			    sav->alg_enc != SADB_EALG_NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: invalid ESP algorithm.\n"));
				error = EINVAL;
				break;
			}
			sav->key_enc = (struct sadb_key *)key_newbuf(key0, len);
			if (sav->key_enc == NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
			break;
		case SADB_SATYPE_AH:
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid key_enc value.\n"));
			goto fail;
		}
	}

	            
	sav->ivlen = 0;

	switch (mhp->msg->sadb_msg_satype) {
	case SADB_SATYPE_ESP:
#if IPSEC_ESP
		algo = esp_algorithm_lookup(sav->alg_enc);
		if (algo && algo->ivlen) {
			sav->ivlen = (*algo->ivlen)(algo, sav);
		}
		if (sav->ivlen == 0) {
			break;
		}
		KMALLOC_NOWAIT(sav->iv, caddr_t, sav->ivlen);
		if (sav->iv == 0) {
			lck_mtx_unlock(sadb_mutex);
			KMALLOC_WAIT(sav->iv, caddr_t, sav->ivlen);
			lck_mtx_lock(sadb_mutex);
			if (sav->iv == 0) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
		}

		                
		if (sav->alg_enc == SADB_X_EALG_AES_GCM) {
			bzero(sav->iv, sav->ivlen);
		} else {
			key_randomfill(sav->iv, sav->ivlen);
		}
#endif
		break;
	case SADB_SATYPE_AH:
		break;
	default:
		ipseclog((LOG_DEBUG, "key_setsaval: invalid SA type.\n"));
		error = EINVAL;
		goto fail;
	}

	                   
	microtime(&tv);
	sav->created = tv.tv_sec;

	                               
	KMALLOC_NOWAIT(sav->lft_c, struct sadb_lifetime *,
	    sizeof(struct sadb_lifetime));
	if (sav->lft_c == NULL) {
		lck_mtx_unlock(sadb_mutex);
		KMALLOC_WAIT(sav->lft_c, struct sadb_lifetime *,
		    sizeof(struct sadb_lifetime));
		lck_mtx_lock(sadb_mutex);
		if (sav->lft_c == NULL) {
			ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
	}

	microtime(&tv);

	sav->lft_c->sadb_lifetime_len =
	    PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	sav->lft_c->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	sav->lft_c->sadb_lifetime_allocations = 0;
	sav->lft_c->sadb_lifetime_bytes = 0;
	sav->lft_c->sadb_lifetime_addtime = tv.tv_sec;
	sav->lft_c->sadb_lifetime_usetime = 0;

	                                 
	{
		const struct sadb_lifetime *lft0;

		lft0 = (struct sadb_lifetime *)
		    (void *)mhp->ext[SADB_EXT_LIFETIME_HARD];
		if (lft0 != NULL) {
			if (mhp->extlen[SADB_EXT_LIFETIME_HARD] < sizeof(*lft0)) {
				ipseclog((LOG_DEBUG, "key_setsaval: invalid hard lifetime ext len.\n"));
				error = EINVAL;
				goto fail;
			}
			sav->lft_h = (struct sadb_lifetime *)key_newbuf(lft0,
			    sizeof(*lft0));
			if (sav->lft_h == NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
			                        
		}

		lft0 = (struct sadb_lifetime *)
		    (void *)mhp->ext[SADB_EXT_LIFETIME_SOFT];
		if (lft0 != NULL) {
			if (mhp->extlen[SADB_EXT_LIFETIME_SOFT] < sizeof(*lft0)) {
				ipseclog((LOG_DEBUG, "key_setsaval: invalid soft lifetime ext len.\n"));
				error = EINVAL;
				goto fail;
			}
			sav->lft_s = (struct sadb_lifetime *)key_newbuf(lft0,
			    sizeof(*lft0));
			if (sav->lft_s == NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
			                        
		}
	}

	return 0;

fail:
	key_reset_sav(sav);
	return error;
}

  
                                                                
                
               
   
static int
key_mature(
	struct secasvar *sav)
{
	int mature;
	int checkmask = 0;                                           
	int mustmask = 0;                                            

	mature = 0;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                     
	switch (sav->sah->saidx.proto) {
	case IPPROTO_ESP:
	case IPPROTO_AH:

		                                                                             
		if (ntohl(sav->spi) <= 255) {
			ipseclog((LOG_DEBUG,
			    "key_mature: illegal range of SPI %u.\n",
			    (u_int32_t)ntohl(sav->spi)));
			return EINVAL;
		}
		break;
	}

	                  
	switch (sav->sah->saidx.proto) {
	case IPPROTO_ESP:
		                 
		if ((sav->flags & SADB_X_EXT_OLD)
		    && (sav->flags & SADB_X_EXT_DERIV)) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "invalid flag (derived) given to old-esp.\n"));
			return EINVAL;
		}
		if (sav->alg_auth == SADB_AALG_NONE) {
			checkmask = 1;
		} else {
			checkmask = 3;
		}
		mustmask = 1;
		break;
	case IPPROTO_AH:
		                 
		if (sav->flags & SADB_X_EXT_DERIV) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "invalid flag (derived) given to AH SA.\n"));
			return EINVAL;
		}
		if (sav->alg_enc != SADB_EALG_NONE) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "protocol and algorithm mismated.\n"));
			return EINVAL;
		}
		checkmask = 2;
		mustmask = 2;
		break;
	default:
		ipseclog((LOG_DEBUG, "key_mature: Invalid satype.\n"));
		return EPROTONOSUPPORT;
	}

	                                    
	if ((checkmask & 2) != 0) {
		const struct ah_algorithm *algo;
		int keylen;

		algo = ah_algorithm_lookup(sav->alg_auth);
		if (!algo) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "unknown authentication algorithm.\n"));
			return EINVAL;
		}

		                               
		if (sav->key_auth) {
			keylen = sav->key_auth->sadb_key_bits;
		} else {
			keylen = 0;
		}
		if (keylen < algo->keymin || algo->keymax < keylen) {
			ipseclog((LOG_DEBUG,
			    "key_mature: invalid AH key length %d "
			    "(%d-%d allowed)\n",
			    keylen, algo->keymin, algo->keymax));
			return EINVAL;
		}

		if (algo->mature) {
			if ((*algo->mature)(sav)) {
				                                                
				return EINVAL;
			} else {
				mature = SADB_SATYPE_AH;
			}
		}

		if ((mustmask & 2) != 0 && mature != SADB_SATYPE_AH) {
			ipseclog((LOG_DEBUG, "key_mature: no satisfy algorithm for AH\n"));
			return EINVAL;
		}
	}

	                                
	if ((checkmask & 1) != 0) {
#if IPSEC_ESP
		const struct esp_algorithm *algo;
		int keylen;

		algo = esp_algorithm_lookup(sav->alg_enc);
		if (!algo) {
			ipseclog((LOG_DEBUG, "key_mature: unknown encryption algorithm.\n"));
			return EINVAL;
		}

		                               
		if (sav->key_enc) {
			keylen = sav->key_enc->sadb_key_bits;
		} else {
			keylen = 0;
		}
		if (keylen < algo->keymin || algo->keymax < keylen) {
			ipseclog((LOG_DEBUG,
			    "key_mature: invalid ESP key length %d "
			    "(%d-%d allowed)\n",
			    keylen, algo->keymin, algo->keymax));
			return EINVAL;
		}

		if (algo->mature) {
			if ((*algo->mature)(sav)) {
				                                                
				return EINVAL;
			} else {
				mature = SADB_SATYPE_ESP;
			}
		}

		if ((mustmask & 1) != 0 && mature != SADB_SATYPE_ESP) {
			ipseclog((LOG_DEBUG, "key_mature: no satisfy algorithm for ESP\n"));
			return EINVAL;
		}
#else              
		ipseclog((LOG_DEBUG, "key_mature: ESP not supported in this configuration\n"));
		return EINVAL;
#endif
	}

	key_sa_chgstate(sav, SADB_SASTATE_MATURE);

	return 0;
}

  
                                         
   
static struct mbuf *
key_setdumpsa(
	struct secasvar *sav,
	u_int8_t type,
	u_int8_t satype,
	u_int32_t seq,
	u_int32_t pid)
{
	struct mbuf *result = NULL, *tres = NULL, *m;
	int l = 0;
	int i;
	void *p;
	int dumporder[] = {
		SADB_EXT_SA, SADB_X_EXT_SA2,
		SADB_EXT_LIFETIME_HARD, SADB_EXT_LIFETIME_SOFT,
		SADB_EXT_LIFETIME_CURRENT, SADB_EXT_ADDRESS_SRC,
		SADB_EXT_ADDRESS_DST, SADB_EXT_ADDRESS_PROXY, SADB_EXT_KEY_AUTH,
		SADB_EXT_KEY_ENCRYPT, SADB_EXT_IDENTITY_SRC,
		SADB_EXT_IDENTITY_DST, SADB_EXT_SENSITIVITY,
	};

	m = key_setsadbmsg(type, 0, satype, seq, pid, (u_int16_t)sav->refcnt);
	if (m == NULL) {
		goto fail;
	}
	result = m;

	for (i = sizeof(dumporder) / sizeof(dumporder[0]) - 1; i >= 0; i--) {
		m = NULL;
		p = NULL;
		switch (dumporder[i]) {
		case SADB_EXT_SA:
			m = key_setsadbsa(sav);
			if (!m) {
				goto fail;
			}
			break;

		case SADB_X_EXT_SA2:
			m = key_setsadbxsa2(sav->sah->saidx.mode,
			    sav->replay[0] ? sav->replay[0]->count : 0,
			    sav->sah->saidx.reqid,
			    sav->flags2);
			if (!m) {
				goto fail;
			}
			break;

		case SADB_EXT_ADDRESS_SRC:
			m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
			    (struct sockaddr *)&sav->sah->saidx.src,
			    FULLMASK, IPSEC_ULPROTO_ANY);
			if (!m) {
				goto fail;
			}
			break;

		case SADB_EXT_ADDRESS_DST:
			m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
			    (struct sockaddr *)&sav->sah->saidx.dst,
			    FULLMASK, IPSEC_ULPROTO_ANY);
			if (!m) {
				goto fail;
			}
			break;

		case SADB_EXT_KEY_AUTH:
			if (!sav->key_auth) {
				continue;
			}
			l = PFKEY_UNUNIT64(sav->key_auth->sadb_key_len);
			p = sav->key_auth;
			break;

		case SADB_EXT_KEY_ENCRYPT:
			if (!sav->key_enc) {
				continue;
			}
			l = PFKEY_UNUNIT64(sav->key_enc->sadb_key_len);
			p = sav->key_enc;
			break;

		case SADB_EXT_LIFETIME_CURRENT:
			if (!sav->lft_c) {
				continue;
			}
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_c)->sadb_ext_len);
			p = sav->lft_c;
			break;

		case SADB_EXT_LIFETIME_HARD:
			if (!sav->lft_h) {
				continue;
			}
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_h)->sadb_ext_len);
			p = sav->lft_h;
			break;

		case SADB_EXT_LIFETIME_SOFT:
			if (!sav->lft_s) {
				continue;
			}
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_s)->sadb_ext_len);
			p = sav->lft_s;
			break;

		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		                                       
		case SADB_EXT_SENSITIVITY:
		default:
			continue;
		}

		if ((!m && !p) || (m && p)) {
			goto fail;
		}
		if (p && tres) {
			M_PREPEND(tres, l, M_WAITOK, 1);
			if (!tres) {
				goto fail;
			}
			bcopy(p, mtod(tres, caddr_t), l);
			continue;
		}
		if (p) {
			m = key_alloc_mbuf(l);
			if (!m) {
				goto fail;
			}
			m_copyback(m, 0, l, p);
		}

		if (tres) {
			m_cat(m, tres);
		}
		tres = m;
	}

	m_cat(result, tres);

	if (sav->sah && (sav->sah->outgoing_if || sav->sah->ipsec_if)) {
		m = key_setsadbipsecif(NULL, ifindex2ifnet[sav->sah->outgoing_if], sav->sah->ipsec_if, 0);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	VERIFY(PFKEY_UNIT64(result->m_pkthdr.len) <= UINT16_MAX);
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return result;

fail:
	m_freem(result);
	m_freem(tres);
	return NULL;
}

  
                          
   
static struct mbuf *
key_setsadbmsg(
	u_int8_t type,
	u_int16_t tlen,
	u_int8_t satype,
	u_int32_t seq,
	pid_t pid,
	u_int16_t reserved)
{
	struct mbuf *m;
	struct sadb_msg *p;
	int len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
	if (len > MCLBYTES) {
		return NULL;
	}
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		}
	}
	if (!m) {
		return NULL;
	}
	m->m_pkthdr.len = m->m_len = len;
	m->m_next = NULL;

	p = mtod(m, struct sadb_msg *);

	bzero(p, len);
	p->sadb_msg_version = PF_KEY_V2;
	p->sadb_msg_type = type;
	p->sadb_msg_errno = 0;
	p->sadb_msg_satype = satype;
	p->sadb_msg_len = PFKEY_UNIT64(tlen);
	p->sadb_msg_reserved = reserved;
	p->sadb_msg_seq = seq;
	p->sadb_msg_pid = (u_int32_t)pid;

	return m;
}

  
                                        
   
static struct mbuf *
key_setsadbsa(
	struct secasvar *sav)
{
	struct mbuf *m;
	struct sadb_sa *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_sa));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, struct sadb_sa *);

	bzero(p, len);
	p->sadb_sa_len = PFKEY_UNIT64(len);
	p->sadb_sa_exttype = SADB_EXT_SA;
	p->sadb_sa_spi = sav->spi;
	p->sadb_sa_replay = (sav->replay[0] != NULL ? sav->replay[0]->wsize : 0);
	p->sadb_sa_state = sav->state;
	p->sadb_sa_auth = sav->alg_auth;
	p->sadb_sa_encrypt = sav->alg_enc;
	p->sadb_sa_flags = sav->flags;

	return m;
}

  
                              
   
static struct mbuf *
key_setsadbaddr(
	u_int16_t exttype,
	struct sockaddr *saddr,
	size_t prefixlen,
	u_int8_t ul_proto)
{
	struct mbuf *m;
	struct sadb_address *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_address)) +
	    PFKEY_ALIGN8(saddr->sa_len);
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, struct sadb_address *);

	bzero(p, len);
	p->sadb_address_len = PFKEY_UNIT64(len);
	p->sadb_address_exttype = exttype;
	p->sadb_address_proto = ul_proto;
	if (prefixlen == FULLMASK) {
		switch (saddr->sa_family) {
		case AF_INET:
			prefixlen = sizeof(struct in_addr) << 3;
			break;
		case AF_INET6:
			prefixlen = sizeof(struct in6_addr) << 3;
			break;
		default:
			;                
		}
	}
	if (prefixlen >= UINT8_MAX) {
		ipseclog((LOG_ERR, "key_setsadbaddr: bad prefix length %zu", prefixlen));
		m_freem(m);
		return NULL;
	}
	p->sadb_address_prefixlen = (u_int8_t)prefixlen;
	p->sadb_address_reserved = 0;

	bcopy(saddr,
	    mtod(m, caddr_t) + PFKEY_ALIGN8(sizeof(struct sadb_address)),
	    saddr->sa_len);

	return m;
}

static struct mbuf *
key_setsadbipsecif(ifnet_t internal_if,
    ifnet_t outgoing_if,
    ifnet_t ipsec_if,
    u_int8_t init_disabled)
{
	struct mbuf *m;
	struct sadb_x_ipsecif *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_ipsecif));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, struct sadb_x_ipsecif *);

	bzero(p, len);
	p->sadb_x_ipsecif_len = PFKEY_UNIT64(len);
	p->sadb_x_ipsecif_exttype = SADB_X_EXT_IPSECIF;

	if (internal_if && internal_if->if_xname) {
		strlcpy(p->sadb_x_ipsecif_internal_if, internal_if->if_xname, IFXNAMSIZ);
	}
	if (outgoing_if && outgoing_if->if_xname) {
		strlcpy(p->sadb_x_ipsecif_outgoing_if, outgoing_if->if_xname, IFXNAMSIZ);
	}
	if (ipsec_if && ipsec_if->if_xname) {
		strlcpy(p->sadb_x_ipsecif_ipsec_if, ipsec_if->if_xname, IFXNAMSIZ);
	}

	p->sadb_x_ipsecif_init_disabled = init_disabled;

	return m;
}

  
                                
   
static struct mbuf *
key_setsadbsession_id(u_int64_t session_ids[])
{
	struct mbuf *m;
	struct sadb_session_id *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(*p));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, __typeof__(p));

	bzero(p, len);
	p->sadb_session_id_len = PFKEY_UNIT64(len);
	p->sadb_session_id_exttype = SADB_EXT_SESSION_ID;
	p->sadb_session_id_v[0] = session_ids[0];
	p->sadb_session_id_v[1] = session_ids[1];

	return m;
}

  
                                         
   
static struct mbuf *
key_setsadbsastat(u_int32_t      dir,
    struct sastat *stats,
    u_int32_t      max_stats)
{
	struct mbuf *m;
	struct sadb_sastat *p;
	size_t list_len, len;

	if (!stats) {
		return NULL;
	}

	list_len = sizeof(*stats) * max_stats;
	len = PFKEY_ALIGN8(sizeof(*p)) + PFKEY_ALIGN8(list_len);
	if (PFKEY_UNIT64(len) >= UINT16_MAX) {
		ipseclog((LOG_ERR, "key_setsadbsastat: length is too big: %zu\n", len));
		return NULL;
	}

	m = key_alloc_mbuf((int)len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, __typeof__(p));

	bzero(p, len);
	p->sadb_sastat_len      = (u_int16_t)PFKEY_UNIT64(len);
	p->sadb_sastat_exttype  = SADB_EXT_SASTAT;
	p->sadb_sastat_dir      = dir;
	p->sadb_sastat_list_len = max_stats;
	if (list_len) {
		bcopy(stats,
		    mtod(m, caddr_t) + PFKEY_ALIGN8(sizeof(*p)),
		    list_len);
	}

	return m;
}

  
                            
   
static struct mbuf *
key_setsadbxsa2(
	u_int8_t mode,
	u_int32_t seq,
	u_int32_t reqid,
	u_int16_t flags)
{
	struct mbuf *m;
	struct sadb_x_sa2 *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_sa2));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, struct sadb_x_sa2 *);

	bzero(p, len);
	p->sadb_x_sa2_len = PFKEY_UNIT64(len);
	p->sadb_x_sa2_exttype = SADB_X_EXT_SA2;
	p->sadb_x_sa2_mode = mode;
	p->sadb_x_sa2_reserved1 = 0;
	p->sadb_x_sa2_reserved2 = 0;
	p->sadb_x_sa2_sequence = seq;
	p->sadb_x_sa2_reqid = reqid;
	p->sadb_x_sa2_flags = flags;

	return m;
}

  
                              
   
static struct mbuf *
key_setsadbxpolicy(
	u_int16_t type,
	u_int8_t dir,
	u_int32_t id)
{
	struct mbuf *m;
	struct sadb_x_policy *p;
	u_int16_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_policy));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		return NULL;
	}

	p = mtod(m, struct sadb_x_policy *);

	bzero(p, len);
	p->sadb_x_policy_len = PFKEY_UNIT64(len);
	p->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	p->sadb_x_policy_type = type;
	p->sadb_x_policy_dir = dir;
	p->sadb_x_policy_id = id;

	return m;
}

                   
  
                                               
   
static void *
key_newbuf(
	const void *src,
	u_int len)
{
	caddr_t new;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);
	KMALLOC_NOWAIT(new, caddr_t, len);
	if (new == NULL) {
		lck_mtx_unlock(sadb_mutex);
		KMALLOC_WAIT(new, caddr_t, len);
		lck_mtx_lock(sadb_mutex);
		if (new == NULL) {
			ipseclog((LOG_DEBUG, "key_newbuf: No more memory.\n"));
			return NULL;
		}
	}
	bcopy(src, new, len);

	return new;
}

                         
                                 
           
   
int
key_ismyaddr(
	struct sockaddr *sa)
{
#if INET
	struct sockaddr_in *sin;
	struct in_ifaddr *ia;
#endif

	                  
	if (sa == NULL) {
		panic("key_ismyaddr: NULL pointer is passed.\n");
	}

	switch (sa->sa_family) {
#if INET
	case AF_INET:
		lck_rw_lock_shared(in_ifaddr_rwlock);
		sin = (struct sockaddr_in *)(void *)sa;
		for (ia = in_ifaddrhead.tqh_first; ia;
		    ia = ia->ia_link.tqe_next) {
			IFA_LOCK_SPIN(&ia->ia_ifa);
			if (sin->sin_family == ia->ia_addr.sin_family &&
			    sin->sin_len == ia->ia_addr.sin_len &&
			    sin->sin_addr.s_addr == ia->ia_addr.sin_addr.s_addr) {
				IFA_UNLOCK(&ia->ia_ifa);
				lck_rw_done(in_ifaddr_rwlock);
				return 1;
			}
			IFA_UNLOCK(&ia->ia_ifa);
		}
		lck_rw_done(in_ifaddr_rwlock);
		break;
#endif
	case AF_INET6:
		return key_ismyaddr6((struct sockaddr_in6 *)(void *)sa);
	}

	return 0;
}

  
                                   
          
           
                                                                       
   
#include <netinet6/in6_var.h>

static int
key_ismyaddr6(
	struct sockaddr_in6 *sin6)
{
	struct in6_ifaddr *ia;
	struct in6_multi *in6m;

	lck_rw_lock_shared(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
		IFA_LOCK(&ia->ia_ifa);
		if (key_sockaddrcmp((struct sockaddr *)&sin6,
		    (struct sockaddr *)&ia->ia_addr, 0) == 0) {
			IFA_UNLOCK(&ia->ia_ifa);
			lck_rw_done(&in6_ifaddr_rwlock);
			return 1;
		}
		IFA_UNLOCK(&ia->ia_ifa);

		  
                  
                                                                 
                           
              
     
		in6m = NULL;
		in6_multihead_lock_shared();
		IN6_LOOKUP_MULTI(&sin6->sin6_addr, ia->ia_ifp, in6m);
		in6_multihead_lock_done();
		if (in6m != NULL) {
			lck_rw_done(&in6_ifaddr_rwlock);
			IN6M_REMREF(in6m);
			return 1;
		}
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	                               
	if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)) {
		return 1;
	}

	return 0;
}

  
                                    
                                         
                                                                
                      
      
                                         
                       
       
                 
                     
   
static int
key_cmpsaidx(
	struct secasindex *saidx0,
	struct secasindex *saidx1,
	int flag)
{
	            
	if (saidx0 == NULL && saidx1 == NULL) {
		return 1;
	}

	if (saidx0 == NULL || saidx1 == NULL) {
		return 0;
	}

	if (saidx0->ipsec_ifindex != 0 && saidx0->ipsec_ifindex != saidx1->ipsec_ifindex) {
		return 0;
	}

	if (saidx0->proto != saidx1->proto) {
		return 0;
	}

	if (flag == CMP_EXACTLY) {
		if (saidx0->mode != saidx1->mode) {
			return 0;
		}
		if (saidx0->reqid != saidx1->reqid) {
			return 0;
		}
		if (bcmp(&saidx0->src, &saidx1->src, saidx0->src.ss_len) != 0 ||
		    bcmp(&saidx0->dst, &saidx1->dst, saidx0->dst.ss_len) != 0) {
			return 0;
		}
	} else {
		                                         
		if (flag & CMP_REQID) {
			  
                                                         
                                                    
      
			if (saidx1->reqid != 0 && saidx0->reqid != saidx1->reqid) {
				return 0;
			}
		}

		if (flag & CMP_MODE) {
			if (saidx0->mode != IPSEC_MODE_ANY
			    && saidx0->mode != saidx1->mode) {
				return 0;
			}
		}

		if (key_sockaddrcmp((struct sockaddr *)&saidx0->src,
		    (struct sockaddr *)&saidx1->src, flag & CMP_PORT ? 1 : 0) != 0) {
			return 0;
		}
		if (key_sockaddrcmp((struct sockaddr *)&saidx0->dst,
		    (struct sockaddr *)&saidx1->dst, flag & CMP_PORT ? 1 : 0) != 0) {
			return 0;
		}
	}

	return 1;
}

  
                                          
      
                                      
                                                  
       
            
                
   
static int
key_cmpspidx_exactly(
	struct secpolicyindex *spidx0,
	struct secpolicyindex *spidx1)
{
	            
	if (spidx0 == NULL && spidx1 == NULL) {
		return 1;
	}

	if (spidx0 == NULL || spidx1 == NULL) {
		return 0;
	}

	if (spidx0->prefs != spidx1->prefs
	    || spidx0->prefd != spidx1->prefd
	    || spidx0->ul_proto != spidx1->ul_proto
	    || spidx0->internal_if != spidx1->internal_if) {
		return 0;
	}

	if (key_sockaddrcmp((struct sockaddr *)&spidx0->src,
	    (struct sockaddr *)&spidx1->src, 1) != 0) {
		return 0;
	}
	if (key_sockaddrcmp((struct sockaddr *)&spidx0->dst,
	    (struct sockaddr *)&spidx1->dst, 1) != 0) {
		return 0;
	}

	if (key_sockaddrcmp((struct sockaddr *)&spidx0->src_range.start,
	    (struct sockaddr *)&spidx1->src_range.start, 1) != 0) {
		return 0;
	}
	if (key_sockaddrcmp((struct sockaddr *)&spidx0->src_range.end,
	    (struct sockaddr *)&spidx1->src_range.end, 1) != 0) {
		return 0;
	}
	if (key_sockaddrcmp((struct sockaddr *)&spidx0->dst_range.start,
	    (struct sockaddr *)&spidx1->dst_range.start, 1) != 0) {
		return 0;
	}
	if (key_sockaddrcmp((struct sockaddr *)&spidx0->dst_range.end,
	    (struct sockaddr *)&spidx1->dst_range.end, 1) != 0) {
		return 0;
	}

	return 1;
}

  
                                            
      
                                      
                                              
       
            
                
   
static int
key_cmpspidx_withmask(
	struct secpolicyindex *spidx0,
	struct secpolicyindex *spidx1)
{
	int spidx0_src_is_range = 0;
	int spidx0_dst_is_range = 0;

	            
	if (spidx0 == NULL && spidx1 == NULL) {
		return 1;
	}

	if (spidx0 == NULL || spidx1 == NULL) {
		return 0;
	}

	if (spidx0->src_range.start.ss_len > 0) {
		spidx0_src_is_range = 1;
	}

	if (spidx0->dst_range.start.ss_len > 0) {
		spidx0_dst_is_range = 1;
	}

	if ((spidx0_src_is_range ? spidx0->src_range.start.ss_family : spidx0->src.ss_family) != spidx1->src.ss_family ||
	    (spidx0_dst_is_range ? spidx0->dst_range.start.ss_family : spidx0->dst.ss_family) != spidx1->dst.ss_family ||
	    (spidx0_src_is_range ? spidx0->src_range.start.ss_len : spidx0->src.ss_len) != spidx1->src.ss_len ||
	    (spidx0_dst_is_range ? spidx0->dst_range.start.ss_len : spidx0->dst.ss_len) != spidx1->dst.ss_len) {
		return 0;
	}

	                                                     
	if (spidx0->ul_proto != (u_int16_t)IPSEC_ULPROTO_ANY
	    && spidx0->ul_proto != spidx1->ul_proto) {
		return 0;
	}

	                                                    
	if (spidx1->internal_if != NULL) {
		if (spidx0->internal_if == NULL
		    || spidx0->internal_if != spidx1->internal_if) {
			return 0;
		}

		                       
		switch (spidx0->src.ss_family) {
		case AF_INET:
			if (spidx0_src_is_range &&
			    (satosin(&spidx1->src)->sin_port < satosin(&spidx0->src_range.start)->sin_port
			    || satosin(&spidx1->src)->sin_port > satosin(&spidx0->src_range.end)->sin_port)) {
				return 0;
			} else if (satosin(&spidx0->src)->sin_port != IPSEC_PORT_ANY
			    && satosin(&spidx0->src)->sin_port !=
			    satosin(&spidx1->src)->sin_port) {
				return 0;
			}
			break;
		case AF_INET6:
			if (spidx0_src_is_range &&
			    (satosin6(&spidx1->src)->sin6_port < satosin6(&spidx0->src_range.start)->sin6_port
			    || satosin6(&spidx1->src)->sin6_port > satosin6(&spidx0->src_range.end)->sin6_port)) {
				return 0;
			} else if (satosin6(&spidx0->src)->sin6_port != IPSEC_PORT_ANY
			    && satosin6(&spidx0->src)->sin6_port !=
			    satosin6(&spidx1->src)->sin6_port) {
				return 0;
			}
			break;
		default:
			break;
		}
	} else if (spidx0_src_is_range) {
		if (!key_is_addr_in_range(&spidx1->src, &spidx0->src_range)) {
			return 0;
		}
	} else {
		switch (spidx0->src.ss_family) {
		case AF_INET:
			if (satosin(&spidx0->src)->sin_port != IPSEC_PORT_ANY
			    && satosin(&spidx0->src)->sin_port !=
			    satosin(&spidx1->src)->sin_port) {
				return 0;
			}
			if (!key_bbcmp((caddr_t)&satosin(&spidx0->src)->sin_addr,
			    (caddr_t)&satosin(&spidx1->src)->sin_addr, spidx0->prefs)) {
				return 0;
			}
			break;
		case AF_INET6:
			if (satosin6(&spidx0->src)->sin6_port != IPSEC_PORT_ANY
			    && satosin6(&spidx0->src)->sin6_port !=
			    satosin6(&spidx1->src)->sin6_port) {
				return 0;
			}
			  
                                                         
                                                           
      
			if (satosin6(&spidx0->src)->sin6_scope_id &&
			    satosin6(&spidx1->src)->sin6_scope_id &&
			    satosin6(&spidx0->src)->sin6_scope_id !=
			    satosin6(&spidx1->src)->sin6_scope_id) {
				return 0;
			}
			if (!key_bbcmp((caddr_t)&satosin6(&spidx0->src)->sin6_addr,
			    (caddr_t)&satosin6(&spidx1->src)->sin6_addr, spidx0->prefs)) {
				return 0;
			}
			break;
		default:
			         
			if (bcmp(&spidx0->src, &spidx1->src, spidx0->src.ss_len) != 0) {
				return 0;
			}
			break;
		}
	}

	if (spidx0_dst_is_range) {
		if (!key_is_addr_in_range(&spidx1->dst, &spidx0->dst_range)) {
			return 0;
		}
	} else {
		switch (spidx0->dst.ss_family) {
		case AF_INET:
			if (satosin(&spidx0->dst)->sin_port != IPSEC_PORT_ANY
			    && satosin(&spidx0->dst)->sin_port !=
			    satosin(&spidx1->dst)->sin_port) {
				return 0;
			}
			if (!key_bbcmp((caddr_t)&satosin(&spidx0->dst)->sin_addr,
			    (caddr_t)&satosin(&spidx1->dst)->sin_addr, spidx0->prefd)) {
				return 0;
			}
			break;
		case AF_INET6:
			if (satosin6(&spidx0->dst)->sin6_port != IPSEC_PORT_ANY
			    && satosin6(&spidx0->dst)->sin6_port !=
			    satosin6(&spidx1->dst)->sin6_port) {
				return 0;
			}
			  
                                                         
                                                           
      
			if (satosin6(&spidx0->src)->sin6_scope_id &&
			    satosin6(&spidx1->src)->sin6_scope_id &&
			    satosin6(&spidx0->dst)->sin6_scope_id !=
			    satosin6(&spidx1->dst)->sin6_scope_id) {
				return 0;
			}
			if (!key_bbcmp((caddr_t)&satosin6(&spidx0->dst)->sin6_addr,
			    (caddr_t)&satosin6(&spidx1->dst)->sin6_addr, spidx0->prefd)) {
				return 0;
			}
			break;
		default:
			         
			if (bcmp(&spidx0->dst, &spidx1->dst, spidx0->dst.ss_len) != 0) {
				return 0;
			}
			break;
		}
	}

	                                                  

	return 1;
}

static int
key_is_addr_in_range(struct sockaddr_storage *addr, struct secpolicyaddrrange *addr_range)
{
	int cmp = 0;

	if (addr == NULL || addr_range == NULL) {
		return 0;
	}

	                                            
	cmp = key_sockaddrcmp((struct sockaddr *)addr, (struct sockaddr *)&addr_range->start, 1);
	if (cmp != 0 && cmp != 1) {
		return 0;
	}

	                                       
	cmp = key_sockaddrcmp((struct sockaddr *)addr, (struct sockaddr *)&addr_range->end, 1);
	if (cmp != 0 && cmp != -1) {
		return 0;
	}

	return 1;
}

  
                  
                 
                 
                
                              
   
static int
key_sockaddrcmp(
	struct sockaddr *sa1,
	struct sockaddr *sa2,
	int port)
{
	int result = 0;
	int port_result = 0;

	if (sa1->sa_family != sa2->sa_family || sa1->sa_len != sa2->sa_len) {
		return 2;
	}

	if (sa1->sa_len == 0) {
		return 0;
	}

	switch (sa1->sa_family) {
	case AF_INET:
		if (sa1->sa_len != sizeof(struct sockaddr_in)) {
			return 2;
		}

		result = memcmp(&satosin(sa1)->sin_addr.s_addr, &satosin(sa2)->sin_addr.s_addr, sizeof(satosin(sa1)->sin_addr.s_addr));

		if (port) {
			if (satosin(sa1)->sin_port < satosin(sa2)->sin_port) {
				port_result = -1;
			} else if (satosin(sa1)->sin_port > satosin(sa2)->sin_port) {
				port_result = 1;
			}

			if (result == 0) {
				result = port_result;
			} else if ((result > 0 && port_result < 0) || (result < 0 && port_result > 0)) {
				return 2;
			}
		}

		break;
	case AF_INET6:
		if (sa1->sa_len != sizeof(struct sockaddr_in6)) {
			return 2;           
		}
		if (satosin6(sa1)->sin6_scope_id !=
		    satosin6(sa2)->sin6_scope_id) {
			return 2;
		}

		result = memcmp(&satosin6(sa1)->sin6_addr.s6_addr[0], &satosin6(sa2)->sin6_addr.s6_addr[0], sizeof(struct in6_addr));

		if (port) {
			if (satosin6(sa1)->sin6_port < satosin6(sa2)->sin6_port) {
				port_result = -1;
			} else if (satosin6(sa1)->sin6_port > satosin6(sa2)->sin6_port) {
				port_result = 1;
			}

			if (result == 0) {
				result = port_result;
			} else if ((result > 0 && port_result < 0) || (result < 0 && port_result > 0)) {
				return 2;
			}
		}

		break;
	default:
		result = memcmp(sa1, sa2, sa1->sa_len);
		break;
	}

	if (result < 0) {
		result = -1;
	} else if (result > 0) {
		result = 1;
	}

	return result;
}

  
                                 
      
                
                
                                   
       
            
                
   
static int
key_bbcmp(
	caddr_t p1,
	caddr_t p2,
	u_int bits)
{
	u_int8_t mask;

	                                                              
                                                              

	                          
	if (p1 == NULL || p2 == NULL) {
		return p1 == p2;
	}

	while (bits >= 8) {
		if (*p1++ != *p2++) {
			return 0;
		}
		bits -= 8;
	}

	if (bits > 0) {
		mask = (u_int8_t)(~((1 << (8 - bits)) - 1));
		if ((*p1 & mask) != (*p2 & mask)) {
			return 0;
		}
	}
	return 1;                   
}

  
                
                                                         
                                 
                                     
   
int key_timehandler_debug = 0;
u_int32_t spd_count = 0, sah_count = 0, dead_sah_count = 0, empty_sah_count = 0, larval_sav_count = 0, mature_sav_count = 0, dying_sav_count = 0, dead_sav_count = 0;
u_int64_t total_sav_count = 0;
void
key_timehandler(void)
{
	u_int dir;
	struct timeval tv;
	struct secpolicy **spbuf = NULL, **spptr = NULL;
	struct secasvar **savexbuf = NULL, **savexptr = NULL;
	struct secasvar **savkabuf = NULL, **savkaptr = NULL;
	size_t total_req_size = 0;
	u_int32_t spbufcount = 0, savbufcount = 0, spcount = 0, savexcount = 0, savkacount = 0, cnt;
	int stop_handler = 1;                            

	microtime(&tv);

	                                                 
	                                                                               
	if ((spbufcount = ipsec_policy_count) != 0) {
		if (os_add_overflow(spbufcount, 256, &spbufcount)) {
			ipseclog((LOG_DEBUG, "key_timehandler: spbufcount overflow, ipsec policy count %u.\n", ipsec_policy_count));
			spbufcount = ipsec_policy_count;
		}

		if (os_mul_overflow(spbufcount, sizeof(struct secpolicy *), &total_req_size)) {
			panic("key_timehandler spbuf requested memory overflow %u\n", spbufcount);
		}
		KMALLOC_WAIT(spbuf, struct secpolicy **, total_req_size);
		if (spbuf) {
			spptr = spbuf;
		}
	}
	if ((savbufcount = ipsec_sav_count) != 0) {
		if (os_add_overflow(savbufcount, 512, &savbufcount)) {
			ipseclog((LOG_DEBUG, "key_timehandler: savbufcount overflow, ipsec sa count %u.\n", ipsec_sav_count));
			savbufcount = ipsec_sav_count;
		}
		if (os_mul_overflow(savbufcount, sizeof(struct secasvar *), &total_req_size)) {
			panic("key_timehandler savexbuf requested memory overflow %u\n", savbufcount);
		}
		KMALLOC_WAIT(savexbuf, struct secasvar **, total_req_size);
		if (savexbuf) {
			savexptr = savexbuf;
		}
		KMALLOC_WAIT(savkabuf, struct secasvar **, total_req_size);
		if (savkabuf) {
			savkaptr = savkabuf;
		}
	}
	lck_mtx_lock(sadb_mutex);
	         
	if (spbuf) {
		struct secpolicy *sp, *nextsp;

		for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
			for (sp = LIST_FIRST(&sptree[dir]);
			    sp != NULL;
			    sp = nextsp) {
				                                                                 
				if (sp->policy != IPSEC_POLICY_GENERATE) {
					stop_handler = 0;
				}
				spd_count++;
				nextsp = LIST_NEXT(sp, chain);

				if (sp->state == IPSEC_SPSTATE_DEAD) {
					key_freesp(sp, KEY_SADB_LOCKED);
					continue;
				}

				if (sp->lifetime == 0 && sp->validtime == 0) {
					continue;
				}
				if (spbuf && spcount < spbufcount) {
					                                       
					if ((sp->lifetime
					    && tv.tv_sec - sp->created > sp->lifetime)
					    || (sp->validtime
					    && tv.tv_sec - sp->lastused > sp->validtime)) {
						                    
						sp->state = IPSEC_SPSTATE_DEAD;
						sp->refcnt++;
						*spptr++ = sp;
						spcount++;
					}
				}
			}
		}
	}

	         
	{
		struct secashead *sah, *nextsah;
		struct secasvar *sav, *nextsav;

		for (sah = LIST_FIRST(&sahtree);
		    sah != NULL;
		    sah = nextsah) {
			sah_count++;
			nextsah = LIST_NEXT(sah, chain);

			                                                                
			if (sah->state == SADB_SASTATE_DEAD) {
				key_delsah(sah);
				dead_sah_count++;
				continue;
			}

			if (LIST_FIRST(&sah->savtree[SADB_SASTATE_LARVAL]) == NULL &&
			    LIST_FIRST(&sah->savtree[SADB_SASTATE_MATURE]) == NULL &&
			    LIST_FIRST(&sah->savtree[SADB_SASTATE_DYING]) == NULL &&
			    LIST_FIRST(&sah->savtree[SADB_SASTATE_DEAD]) == NULL) {
				key_delsah(sah);
				empty_sah_count++;
				continue;
			}

			if (savbufcount == 0) {
				continue;
			}

			stop_handler = 0;

			                                                       
			for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_LARVAL]);
			    sav != NULL;
			    sav = nextsav) {
				larval_sav_count++;
				total_sav_count++;
				nextsav = LIST_NEXT(sav, chain);

				if (sav->lft_h != NULL) {
					                                                             
					if (sav->lft_h->sadb_lifetime_addtime != 0
					    && tv.tv_sec - sav->created > sav->lft_h->sadb_lifetime_addtime) {
						if (sav->always_expire) {
							key_send_delete(sav);
							sav = NULL;
						} else {
							key_sa_chgstate(sav, SADB_SASTATE_DEAD);
							key_freesav(sav, KEY_SADB_LOCKED);
							sav = NULL;
						}
					}
				} else {
					if (tv.tv_sec - sav->created > key_larval_lifetime) {
						key_freesav(sav, KEY_SADB_LOCKED);
					}
				}
			}

			  
                                                     
                                   
     
                                                     
                                                   
                                                       
                                                        
                                 
      
			if (savkabuf && savkacount < savbufcount) {
				sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_MATURE]);                                                       
				if (sav && (natt_keepalive_interval || sav->natt_interval) &&
				    (sav->flags & (SADB_X_EXT_NATT_KEEPALIVE | SADB_X_EXT_ESP_KEEPALIVE)) != 0) {
					sav->refcnt++;
					*savkaptr++ = sav;
					savkacount++;
				}
			}

			  
                                                        
                     
      
			for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_MATURE]);
			    sav != NULL;
			    sav = nextsav) {
				mature_sav_count++;
				total_sav_count++;
				nextsav = LIST_NEXT(sav, chain);

				                             
				if (sav->lft_s == NULL) {
					continue;
				}

				                  
				if (sav->lft_c == NULL) {
					ipseclog((LOG_DEBUG, "key_timehandler: "
					    "There is no CURRENT time, why?\n"));
					continue;
				}

				                         
				if (sav->lft_s->sadb_lifetime_addtime != 0
				    && tv.tv_sec - sav->created > sav->lft_s->sadb_lifetime_addtime) {
					  
                                                   
                                                        
        
					if (sav->lft_c->sadb_lifetime_usetime == 0
					    && sav->always_expire == 0) {
						key_sa_chgstate(sav, SADB_SASTATE_DEAD);
						key_freesav(sav, KEY_SADB_LOCKED);
						sav = NULL;
					} else if (savexbuf && savexcount < savbufcount) {
						key_sa_chgstate(sav, SADB_SASTATE_DYING);
						sav->refcnt++;
						*savexptr++ = sav;
						savexcount++;
					}
				}
				                                  
				  
                                                 
                                                   
                                          
       
				else if (savexbuf && savexcount < savbufcount
				    && sav->lft_s->sadb_lifetime_bytes != 0
				    && sav->lft_s->sadb_lifetime_bytes < sav->lft_c->sadb_lifetime_bytes) {
					  
                                     
                                
                                    
        
					                  
					key_sa_chgstate(sav, SADB_SASTATE_DYING);
					sav->refcnt++;
					*savexptr++ = sav;
					savexcount++;
				}
			}

			                                                 
			for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_DYING]);
			    sav != NULL;
			    sav = nextsav) {
				dying_sav_count++;
				total_sav_count++;
				nextsav = LIST_NEXT(sav, chain);

				                             
				if (sav->lft_h == NULL) {
					continue;
				}

				                  
				if (sav->lft_c == NULL) {
					ipseclog((LOG_DEBUG, "key_timehandler: "
					    "There is no CURRENT time, why?\n"));
					continue;
				}

				if (sav->lft_h->sadb_lifetime_addtime != 0
				    && tv.tv_sec - sav->created > sav->lft_h->sadb_lifetime_addtime) {
					if (sav->always_expire) {
						key_send_delete(sav);
						sav = NULL;
					} else {
						key_sa_chgstate(sav, SADB_SASTATE_DEAD);
						key_freesav(sav, KEY_SADB_LOCKED);
						sav = NULL;
					}
				}
				                                  
				else if (sav->lft_h->sadb_lifetime_bytes != 0
				    && sav->lft_h->sadb_lifetime_bytes < sav->lft_c->sadb_lifetime_bytes) {
					key_sa_chgstate(sav, SADB_SASTATE_DEAD);
					key_freesav(sav, KEY_SADB_LOCKED);
					sav = NULL;
				}
			}

			                          
			for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_DEAD]);
			    sav != NULL;
			    sav = nextsav) {
				dead_sav_count++;
				total_sav_count++;
				nextsav = LIST_NEXT(sav, chain);

				                  
				if (sav->state != SADB_SASTATE_DEAD) {
					ipseclog((LOG_DEBUG, "key_timehandler: "
					    "invalid sav->state "
					    "(queue: %d SA: %d): "
					    "kill it anyway\n",
					    SADB_SASTATE_DEAD, sav->state));
				}

				  
                                      
                                                   
                                    
                          
       
			}
		}
	}

	if (++key_timehandler_debug >= 300) {
		if (key_debug_level) {
			printf("%s: total stats for %u calls\n", __FUNCTION__, key_timehandler_debug);
			printf("%s: walked %u SPDs\n", __FUNCTION__, spd_count);
			printf("%s: walked %llu SAs: LARVAL SAs %u, MATURE SAs %u, DYING SAs %u, DEAD SAs %u\n", __FUNCTION__,
			    total_sav_count, larval_sav_count, mature_sav_count, dying_sav_count, dead_sav_count);
			printf("%s: walked %u SAHs: DEAD SAHs %u, EMPTY SAHs %u\n", __FUNCTION__,
			    sah_count, dead_sah_count, empty_sah_count);
			if (sah_search_calls) {
				printf("%s: SAH search cost %d iters per call\n", __FUNCTION__,
				    (sah_search_count / sah_search_calls));
			}
		}
		spd_count = 0;
		sah_count = 0;
		dead_sah_count = 0;
		empty_sah_count = 0;
		larval_sav_count = 0;
		mature_sav_count = 0;
		dying_sav_count = 0;
		dead_sav_count = 0;
		total_sav_count = 0;
		sah_search_count = 0;
		sah_search_calls = 0;
		key_timehandler_debug = 0;
	}
#ifndef IPSEC_NONBLOCK_ACQUIRE
	              
	{
		struct secacq *acq, *nextacq;

		for (acq = LIST_FIRST(&acqtree);
		    acq != NULL;
		    acq = nextacq) {
			stop_handler = 0;
			nextacq = LIST_NEXT(acq, chain);

			if (tv.tv_sec - acq->created > key_blockacq_lifetime
			    && __LIST_CHAINED(acq)) {
				LIST_REMOVE(acq, chain);
				KFREE(acq);
			}
		}
	}
#endif

	                 
	{
		struct secspacq *acq, *nextacq;

		for (acq = LIST_FIRST(&spacqtree);
		    acq != NULL;
		    acq = nextacq) {
			stop_handler = 0;
			nextacq = LIST_NEXT(acq, chain);

			if (tv.tv_sec - acq->created > key_blockacq_lifetime
			    && __LIST_CHAINED(acq)) {
				LIST_REMOVE(acq, chain);
				KFREE(acq);
			}
		}
	}

	                            
	if (key_tick_init_random++ > key_int_random) {
		key_tick_init_random = 0;
		key_srandom();
	}

	uint64_t acc_sleep_time = 0;
	absolutetime_to_nanoseconds(mach_absolutetime_asleep, &acc_sleep_time);
	natt_now = ++up_time + (acc_sleep_time / NSEC_PER_SEC);

	lck_mtx_unlock(sadb_mutex);

	                                         
	if (spbuf && spcount > 0) {
		cnt = spcount;
		while (cnt--) {
			key_spdexpire(*(--spptr));
		}
	}
	if (savkabuf && savkacount > 0) {
		struct secasvar **savkaptr_sav = savkaptr;
		u_int32_t cnt_send = savkacount;

		while (cnt_send--) {
			if (ipsec_send_natt_keepalive(*(--savkaptr))) {
				                                                                  
				struct secasvar **savkaptr_update = savkaptr_sav;
				u_int32_t cnt_update = savkacount;
				while (cnt_update--) {
					key_update_natt_keepalive_timestamp(*savkaptr,
					    *(--savkaptr_update));
				}
			}
		}
	}
	if (savexbuf && savexcount > 0) {
		cnt = savexcount;
		while (cnt--) {
			key_expire(*(--savexptr));
		}
	}

	                                           
	lck_mtx_lock(sadb_mutex);
	if (spbuf) {
		while (spcount--) {
			key_freesp(*spptr++, KEY_SADB_LOCKED);
		}
		KFREE(spbuf);
	}
	if (savkabuf) {
		while (savkacount--) {
			key_freesav(*savkaptr++, KEY_SADB_LOCKED);
		}
		KFREE(savkabuf);
	}
	if (savexbuf) {
		while (savexcount--) {
			key_freesav(*savexptr++, KEY_SADB_LOCKED);
		}
		KFREE(savexbuf);
	}

	if (stop_handler) {
		key_timehandler_running = 0;
		                              
		ipsec_bypass = 1;
	} else {
		                                 
		(void)timeout((void *)key_timehandler, (void *)0, hz);
	}

	lck_mtx_unlock(sadb_mutex);
	return;
}

  
                                    
   
static void
key_srandom(void)
{
#ifdef __APPLE__
	                                                               
	random();
#else
	struct timeval tv;

	microtime(&tv);

	srandom(tv.tv_usec);
#endif

	return;
}

u_int32_t
key_random(void)
{
	u_int32_t value;

	key_randomfill(&value, sizeof(value));
	return value;
}

void
key_randomfill(
	void *p,
	size_t l)
{
#ifdef __APPLE__
	cc_rand_generate(p, l);
#else
	size_t n;
	u_int32_t v;
	static int warn = 1;

	n = 0;
	n = (size_t)read_random(p, (u_int)l);
	                 
	while (n < l) {
		v = random();
		bcopy(&v, (u_int8_t *)p + n,
		    l - n < sizeof(v) ? l - n : sizeof(v));
		n += sizeof(v);

		if (warn) {
			printf("WARNING: pseudo-random number generator "
			    "used for IPsec processing\n");
			warn = 0;
		}
	}
#endif
}

  
                                  
                                                        
       
                     
   
static u_int8_t
key_satype2proto(
	u_int8_t satype)
{
	switch (satype) {
	case SADB_SATYPE_UNSPEC:
		return IPSEC_PROTO_ANY;
	case SADB_SATYPE_AH:
		return IPPROTO_AH;
	case SADB_SATYPE_ESP:
		return IPPROTO_ESP;
	default:
		return 0;
	}
	                
}

  
                                 
       
                            
   
static u_int8_t
key_proto2satype(
	u_int16_t proto)
{
	switch (proto) {
	case IPPROTO_AH:
		return SADB_SATYPE_AH;
	case IPPROTO_ESP:
		return SADB_SATYPE_ESP;
	default:
		return 0;
	}
	                
}

static ifnet_t
key_get_ipsec_if_from_message(const struct sadb_msghdr *mhp, int message_type)
{
	struct sadb_x_ipsecif *ipsecifopts = NULL;
	ifnet_t ipsec_if = NULL;

	ipsecifopts = (struct sadb_x_ipsecif *)(void *)mhp->ext[message_type];
	if (ipsecifopts != NULL) {
		if (ipsecifopts->sadb_x_ipsecif_ipsec_if[0]) {
			ipsecifopts->sadb_x_ipsecif_ipsec_if[IFXNAMSIZ - 1] = '\0';
			ifnet_find_by_name(ipsecifopts->sadb_x_ipsecif_ipsec_if, &ipsec_if);
		}
	}

	return ipsec_if;
}

static u_int
key_get_outgoing_ifindex_from_message(const struct sadb_msghdr *mhp, int message_type)
{
	struct sadb_x_ipsecif *ipsecifopts = NULL;
	ifnet_t outgoing_if = NULL;

	ipsecifopts = (struct sadb_x_ipsecif *)(void *)mhp->ext[message_type];
	if (ipsecifopts != NULL) {
		if (ipsecifopts->sadb_x_ipsecif_outgoing_if[0]) {
			ipsecifopts->sadb_x_ipsecif_outgoing_if[IFXNAMSIZ - 1] = '\0';
			ifnet_find_by_name(ipsecifopts->sadb_x_ipsecif_outgoing_if, &outgoing_if);
		}
	}

	u_int outgoing_if_index = 0;
	if (outgoing_if != NULL) {
		outgoing_if_index = outgoing_if->if_index;
		ifnet_release(outgoing_if);
	}

	return outgoing_if_index;
}

                
  
                                       
                                                       
                                                                       
                                           
                             
                
  
                                                  
                     
                                                           
   
static int
key_getspi(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *newsah;
	struct secasvar *newsav;
	ifnet_t ipsec_if = NULL;
	u_int8_t proto;
	u_int32_t spi;
	u_int8_t mode;
	u_int32_t reqid;
	int error;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_getspi: NULL pointer is passed.\n");
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	                                       
	switch (((struct sockaddr *)(src0 + 1))->sa_family) {
	case AF_INET:
		if (((struct sockaddr *)(src0 + 1))->sa_len !=
		    sizeof(struct sockaddr_in)) {
			return key_senderror(so, m, EINVAL);
		}
		((struct sockaddr_in *)(void *)(src0 + 1))->sin_port = 0;
		break;
	case AF_INET6:
		if (((struct sockaddr *)(src0 + 1))->sa_len !=
		    sizeof(struct sockaddr_in6)) {
			return key_senderror(so, m, EINVAL);
		}
		((struct sockaddr_in6 *)(void *)(src0 + 1))->sin6_port = 0;
		break;
	default:
		;                
	}
	switch (((struct sockaddr *)(dst0 + 1))->sa_family) {
	case AF_INET:
		if (((struct sockaddr *)(dst0 + 1))->sa_len !=
		    sizeof(struct sockaddr_in)) {
			return key_senderror(so, m, EINVAL);
		}
		((struct sockaddr_in *)(void *)(dst0 + 1))->sin_port = 0;
		break;
	case AF_INET6:
		if (((struct sockaddr *)(dst0 + 1))->sa_len !=
		    sizeof(struct sockaddr_in6)) {
			return key_senderror(so, m, EINVAL);
		}
		((struct sockaddr_in6 *)(void *)(dst0 + 1))->sin6_port = 0;
		break;
	default:
		;                
	}

	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	                                       
	KEY_SETSECASIDX(proto, mode, reqid, src0 + 1, dst0 + 1, ipsec_if ? ipsec_if->if_index : 0, &saidx);

	lck_mtx_lock(sadb_mutex);

	                    
	spi = key_do_getnewspi((struct sadb_spirange *)
	    (void *)mhp->ext[SADB_EXT_SPIRANGE], &saidx);
	if (spi == 0) {
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if != NULL) {
			ifnet_release(ipsec_if);
		}
		return key_senderror(so, m, EINVAL);
	}

	                    
	if ((newsah = key_getsah(&saidx, SECURITY_ASSOCIATION_ANY)) == NULL) {
		                                                                      
		if ((newsah = key_newsah(&saidx, ipsec_if, key_get_outgoing_ifindex_from_message(mhp, SADB_X_EXT_IPSECIF), IPSEC_DIR_INBOUND, SECURITY_ASSOCIATION_PFKEY)) == NULL) {
			lck_mtx_unlock(sadb_mutex);
			if (ipsec_if != NULL) {
				ifnet_release(ipsec_if);
			}
			ipseclog((LOG_DEBUG, "key_getspi: No more memory.\n"));
			return key_senderror(so, m, ENOBUFS);
		}
	}

	if (ipsec_if != NULL) {
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                                                        
	newsah->use_count++;

	if ((newsah->flags & SECURITY_ASSOCIATION_CUSTOM_IPSEC) == SECURITY_ASSOCIATION_CUSTOM_IPSEC) {
		newsah->use_count--;
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_ERR, "key_getspi: custom ipsec exists\n"));
		return key_senderror(so, m, EEXIST);
	}

	                  
	                 
	newsav = key_newsav(m, mhp, newsah, &error, so);
	if (newsav == NULL) {
		                                                     
		newsah->use_count--;
		lck_mtx_unlock(sadb_mutex);
		return key_senderror(so, m, error);
	}

	if (newsah->state == SADB_SASTATE_DEAD) {
		newsah->use_count--;
		key_sa_chgstate(newsav, SADB_SASTATE_DEAD);
		key_freesav(newsav, KEY_SADB_LOCKED);
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_ERR, "key_getspi: security association head is dead\n"));
		return key_senderror(so, m, EINVAL);
	}

	             
	key_setspi(newsav, htonl(spi));

#ifndef IPSEC_NONBLOCK_ACQUIRE
	                                 
	if (mhp->msg->sadb_msg_seq != 0) {
		struct secacq *acq;
		if ((acq = key_getacqbyseq(mhp->msg->sadb_msg_seq)) != NULL) {
			                                                        
			struct timeval tv;
			microtime(&tv);
			acq->created = tv.tv_sec;
			acq->count = 0;
		}
	}
#endif
	newsah->use_count--;
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n, *nn;
		struct sadb_sa *m_sa;
		struct sadb_msg *newmsg;
		int off, len;

		                                   
		len = PFKEY_ALIGN8(sizeof(struct sadb_msg)) +
		    PFKEY_ALIGN8(sizeof(struct sadb_sa));
		if (len > MCLBYTES) {
			return key_senderror(so, m, ENOBUFS);
		}

		MGETHDR(n, M_WAITOK, MT_DATA);
		if (n && len > MHLEN) {
			MCLGET(n, M_WAITOK);
			if ((n->m_flags & M_EXT) == 0) {
				m_freem(n);
				n = NULL;
			}
		}
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		n->m_len = len;
		n->m_next = NULL;
		off = 0;

		m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, caddr_t) + off);
		off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

		m_sa = (struct sadb_sa *)(void *)(mtod(n, caddr_t) + off);
		memset(m_sa, 0, PFKEY_ALIGN8(sizeof(struct sadb_sa)));
		m_sa->sadb_sa_len = PFKEY_UNIT64(sizeof(struct sadb_sa));
		m_sa->sadb_sa_exttype = SADB_EXT_SA;
		m_sa->sadb_sa_spi = htonl(spi);
		off += PFKEY_ALIGN8(sizeof(struct sadb_sa));

#if DIAGNOSTIC
		if (off != len) {
			panic("length inconsistency in key_getspi");
		}
#endif
		{
			int mbufItems[] = {SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST};
			n->m_next = key_gather_mbuf(m, mhp, 0, sizeof(mbufItems) / sizeof(int), mbufItems);
			if (!n->m_next) {
				m_freem(n);
				return key_senderror(so, m, ENOBUFS);
			}
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_sendup_mbuf(so, m, KEY_SENDUP_ONE);
			}
		}

		n->m_pkthdr.len = 0;
		for (nn = n; nn; nn = nn->m_next) {
			n->m_pkthdr.len += nn->m_len;
		}

		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_seq = newsav->seq;
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
	}
}

  
                     
                          
       
              
                   
   
static u_int32_t
key_do_getnewspi(
	struct sadb_spirange *spirange,
	struct secasindex *saidx)
{
	u_int32_t newspi;
	u_int32_t keymin, keymax;
	int count = key_spi_trycnt;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	                               
	if (spirange != NULL) {
		keymin = spirange->sadb_spirange_min;
		keymax = spirange->sadb_spirange_max;
	} else {
		keymin = key_spi_minval;
		keymax = key_spi_maxval;
	}
	if (keymin == keymax) {
		if (key_checkspidup(saidx, keymin) != NULL) {
			ipseclog((LOG_DEBUG, "key_do_getnewspi: SPI %u exists already.\n", keymin));
			return 0;
		}

		count--;                       
		newspi = keymin;
	} else {
		u_int32_t range = keymax - keymin + 1;                                               

		              
		newspi = 0;

		                                            
		while (count--) {
			u_int32_t rand_val = key_random();

			                                              
			newspi = (range == 0 ? rand_val : keymin + (rand_val % range));

			if (key_checkspidup(saidx, newspi) == NULL) {
				break;
			}
		}

		if (count == 0 || newspi == 0) {
			ipseclog((LOG_DEBUG, "key_do_getnewspi: to allocate spi is failed.\n"));
			return 0;
		}
	}

	                
	keystat.getspi_count =
	    (keystat.getspi_count + key_spi_trycnt - count) / 2;

	return newspi;
}

  
                         
          
                                                                  
                                                
                                                                                   
           
                                                                  
                                       
                
  
                          
   
static int
key_update(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0 = NULL;
	struct sadb_address *src0 = NULL, *dst0 = NULL;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *sah = NULL;
	struct secasvar *sav = NULL;
	u_int8_t proto;
	u_int8_t mode;
	u_int32_t reqid;
	u_int16_t flags2;
	int error;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_update: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_update: invalid satype is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_ESP &&
	    mhp->ext[SADB_EXT_KEY_ENCRYPT] == NULL) ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_AH &&
	    mhp->ext[SADB_EXT_KEY_AUTH] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL &&
	    mhp->ext[SADB_EXT_LIFETIME_SOFT] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] == NULL &&
	    mhp->ext[SADB_EXT_LIFETIME_SOFT] != NULL)) {
		ipseclog((LOG_DEBUG, "key_update: invalid message is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_update: invalid message is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
		flags2 = ((struct sadb_x_sa2 *)(void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_flags;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
		flags2 = 0;
	}
	                                                

	sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	u_int ipsec_if_index = 0;
	if (ipsec_if != NULL) {
		ipsec_if_index = ipsec_if->if_index;
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                       
	KEY_SETSECASIDX(proto, mode, reqid, src0 + 1, dst0 + 1, ipsec_if_index, &saidx);

	lck_mtx_lock(sadb_mutex);

	                     
	if ((sah = key_getsah(&saidx, SECURITY_ASSOCIATION_PFKEY)) == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_update: no SA index found.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, ENOENT);
	}

	                                                                          
	sah->use_count++;

	if ((sav = key_getsavbyspi(sah, sa0->sadb_sa_spi)) == NULL) {
		ipseclog((LOG_DEBUG,
		    "key_update: no such a SA found (spi:%u)\n",
		    (u_int32_t)ntohl(sa0->sadb_sa_spi)));
		error = EINVAL;
		goto fail;
	}

	                                                                                
	sav->refcnt++;

	                    
	if (sav->sah->saidx.proto != proto) {
		ipseclog((LOG_DEBUG,
		    "key_update: protocol mismatched (DB=%u param=%u)\n",
		    sav->sah->saidx.proto, proto));
		error = EINVAL;
		goto fail;
	}

	if (sav->pid != mhp->msg->sadb_msg_pid) {
		ipseclog((LOG_DEBUG,
		    "key_update: pid mismatched (DB:%u param:%u)\n",
		    sav->pid, mhp->msg->sadb_msg_pid));
		error = EINVAL;
		goto fail;
	}

	                     
	error = key_setsaval(sav, m, mhp);
	if (error) {
		goto fail;
	}

	if (sah->state == SADB_SASTATE_DEAD) {
		ipseclog((LOG_ERR,
		    "key_update: security association head is dead\n"));
		error = EINVAL;
		goto fail;
	}

	sav->flags2 = flags2;
	if (flags2 & SADB_X_EXT_SA2_DELETE_ON_DETACH) {
		sav->so = so;
	}

	  
                                                            
                                                       
    
	if ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0 &&
	    (sav->sah->saidx.mode != IPSEC_MODE_TRANSPORT ||
	    sav->sah->saidx.src.ss_family != AF_INET)) {
		sav->flags &= ~SADB_X_EXT_NATT_MULTIPLEUSERS;
	}

	                                   
	if ((error = key_mature(sav)) != 0) {
		goto fail;
	}

	key_freesav(sav, KEY_SADB_LOCKED);
	sah->use_count--;
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n;

		                          
		n = key_getmsgbuf_x1(m, mhp);
		if (n == NULL) {
			ipseclog((LOG_DEBUG, "key_update: No more memory.\n"));
			return key_senderror(so, m, ENOBUFS);
		}

		bzero_keys(mhp);
		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
fail:
	if (sav != NULL) {
		key_freesav(sav, KEY_SADB_LOCKED);
	}
	if (sah != NULL) {
		sah->use_count--;
	}

	lck_mtx_unlock(sadb_mutex);
	bzero_keys(mhp);
	return key_senderror(so, m, error);
}

static int
key_migrate(struct socket *so,
    struct mbuf *m,
    const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0 = NULL;
	struct sadb_address *src0 = NULL;
	struct sadb_address *dst0 = NULL;
	struct sadb_address *src1 = NULL;
	struct sadb_address *dst1 = NULL;
	ifnet_t ipsec_if0 = NULL;
	ifnet_t ipsec_if1 = NULL;
	struct secasindex saidx0;
	struct secasindex saidx1;
	struct secashead *sah = NULL;
	struct secashead *newsah = NULL;
	struct secasvar *sav = NULL;
	u_int8_t proto;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_migrate: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_migrate: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_EXT_MIGRATE_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_MIGRATE_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_migrate: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_MIGRATE_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_MIGRATE_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_migrate: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	lck_mtx_lock(sadb_mutex);

	sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);
	src1 = (struct sadb_address *)(mhp->ext[SADB_EXT_MIGRATE_ADDRESS_SRC]);
	dst1 = (struct sadb_address *)(mhp->ext[SADB_EXT_MIGRATE_ADDRESS_DST]);
	ipsec_if0 = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);
	ipsec_if1 = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_MIGRATE_IPSECIF);

	u_int ipsec_if0_index = 0;
	if (ipsec_if0 != NULL) {
		ipsec_if0_index = ipsec_if0->if_index;
		ifnet_release(ipsec_if0);
		ipsec_if0 = NULL;
	}

	                               
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, src0 + 1, dst0 + 1, ipsec_if0_index, &saidx0);

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state != SADB_SASTATE_MATURE) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx0, CMP_HEAD) == 0) {
			continue;
		}

		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		if (sav && sav->state == SADB_SASTATE_MATURE) {
			break;
		}
	}
	if (sah == NULL) {
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if1 != NULL) {
			ifnet_release(ipsec_if1);
		}
		ipseclog((LOG_DEBUG, "key_migrate: no mature SAH found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	if (sav == NULL) {
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if1 != NULL) {
			ifnet_release(ipsec_if1);
		}
		ipseclog((LOG_DEBUG, "key_migrate: no SA found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	                            
	KEY_SETSECASIDX(proto, sah->saidx.mode, sah->saidx.reqid, src1 + 1, dst1 + 1, ipsec_if1 ? ipsec_if1->if_index : 0, &saidx1);

	if ((newsah = key_getsah(&saidx1, SECURITY_ASSOCIATION_ANY)) == NULL) {
		if ((newsah = key_newsah(&saidx1, ipsec_if1, key_get_outgoing_ifindex_from_message(mhp, SADB_X_EXT_MIGRATE_IPSECIF), sah->dir, SECURITY_ASSOCIATION_PFKEY)) == NULL) {
			lck_mtx_unlock(sadb_mutex);
			if (ipsec_if1 != NULL) {
				ifnet_release(ipsec_if1);
			}
			ipseclog((LOG_DEBUG, "key_migrate: No more memory.\n"));
			return key_senderror(so, m, ENOBUFS);
		}
	}

	if (ipsec_if1 != NULL) {
		ifnet_release(ipsec_if1);
		ipsec_if1 = NULL;
	}

	if ((newsah->flags & SECURITY_ASSOCIATION_CUSTOM_IPSEC) == SECURITY_ASSOCIATION_CUSTOM_IPSEC) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_ERR, "key_migrate: custom ipsec exists\n"));
		return key_senderror(so, m, EEXIST);
	}

	                               
	if (key_migratesav(sav, newsah) != 0) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_migrate: Failed to migrate SA to new SAH.\n"));
		return key_senderror(so, m, EINVAL);
	}

	                      
	sav->flags = sa0->sadb_sa_flags;
	sav->natt_encapsulated_src_port = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_src_port;
	sav->remote_ike_port = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_port;
	sav->natt_interval = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_interval;
	sav->natt_offload_interval = ((const struct sadb_sa_2*)(sa0))->sadb_sa_natt_offload_interval;
	sav->natt_last_activity = natt_now;

	  
                                                            
                                                               
                                                    
    
	if ((sav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0) {
		if ((sav->flags & SADB_X_EXT_NATT) == 0 ||
		    (sav->flags & SADB_X_EXT_NATT_KEEPALIVE) != 0) {
			sav->flags &= ~SADB_X_EXT_NATT_MULTIPLEUSERS;
		}
	}

	lck_mtx_unlock(sadb_mutex);
	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int mbufItems[] = {SADB_EXT_RESERVED, SADB_EXT_SA,
			           SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST, SADB_X_EXT_IPSECIF,
			           SADB_EXT_MIGRATE_ADDRESS_SRC, SADB_EXT_MIGRATE_ADDRESS_DST, SADB_X_EXT_MIGRATE_IPSECIF};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                      
                                            
                                                                  
                                                
                  
           
                                                                  
                                       
                
  
                                            
  
                          
   
static int
key_add(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0 = NULL;
	struct sadb_address *src0 = NULL, *dst0 = NULL;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *newsah = NULL;
	struct secasvar *newsav = NULL;
	u_int8_t proto;
	u_int8_t mode;
	u_int32_t reqid;
	int error;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_add: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_add: invalid satype is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_ESP &&
	    mhp->ext[SADB_EXT_KEY_ENCRYPT] == NULL) ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_AH &&
	    mhp->ext[SADB_EXT_KEY_AUTH] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL &&
	    mhp->ext[SADB_EXT_LIFETIME_SOFT] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] == NULL &&
	    mhp->ext[SADB_EXT_LIFETIME_SOFT] != NULL)) {
		ipseclog((LOG_DEBUG, "key_add: invalid message is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		                   
		ipseclog((LOG_DEBUG, "key_add: invalid message is passed.\n"));
		bzero_keys(mhp);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)
		    (void *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	                                       
	KEY_SETSECASIDX(proto, mode, reqid, src0 + 1, dst0 + 1, ipsec_if ? ipsec_if->if_index : 0, &saidx);

	lck_mtx_lock(sadb_mutex);

	                     
	if ((newsah = key_getsah(&saidx, SECURITY_ASSOCIATION_ANY)) == NULL) {
		                                                                        
		if ((newsah = key_newsah(&saidx, ipsec_if, key_get_outgoing_ifindex_from_message(mhp, SADB_X_EXT_IPSECIF), IPSEC_DIR_OUTBOUND, SECURITY_ASSOCIATION_PFKEY)) == NULL) {
			ipseclog((LOG_DEBUG, "key_add: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
	}

	if (ipsec_if != NULL) {
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                                                        
	newsah->use_count++;

	if ((newsah->flags & SECURITY_ASSOCIATION_CUSTOM_IPSEC) == SECURITY_ASSOCIATION_CUSTOM_IPSEC) {
		ipseclog((LOG_ERR, "key_add: custom ipsec exists\n"));
		error = EEXIST;
		goto fail;
	}

	                          
	                                                    
	if (key_getsavbyspi(newsah, sa0->sadb_sa_spi)) {
		ipseclog((LOG_DEBUG, "key_add: SA already exists.\n"));
		error = EEXIST;
		goto fail;
	}
	newsav = key_newsav(m, mhp, newsah, &error, so);
	if (newsav == NULL) {
		goto fail;
	}

	if (newsah->state == SADB_SASTATE_DEAD) {
		ipseclog((LOG_ERR, "key_add: security association head is dead\n"));
		error = EINVAL;
		goto fail;
	}

	  
                                                            
                                                       
    
	if ((newsav->flags & SADB_X_EXT_NATT_MULTIPLEUSERS) != 0 &&
	    (newsah->saidx.mode != IPSEC_MODE_TRANSPORT ||
	    newsah->saidx.dst.ss_family != AF_INET)) {
		newsav->flags &= ~SADB_X_EXT_NATT_MULTIPLEUSERS;
	}

	                                   
	if ((error = key_mature(newsav)) != 0) {
		goto fail;
	}

	newsah->use_count--;
	lck_mtx_unlock(sadb_mutex);

	  
                                                                  
                               
    

	{
		struct mbuf *n;

		                          
		n = key_getmsgbuf_x1(m, mhp);
		if (n == NULL) {
			ipseclog((LOG_DEBUG, "key_update: No more memory.\n"));
			bzero_keys(mhp);
			return key_senderror(so, m, ENOBUFS);
		}

		                                     
		                                                     
		bzero_keys(mhp);
		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
fail:
	if (newsav != NULL) {
		key_sa_chgstate(newsav, SADB_SASTATE_DEAD);
		key_freesav(newsav, KEY_SADB_LOCKED);
	}
	if (newsah != NULL) {
		newsah->use_count--;
	}
	lck_mtx_unlock(sadb_mutex);
	if (ipsec_if != NULL) {
		ifnet_release(ipsec_if);
	}
	bzero_keys(mhp);
	return key_senderror(so, m, error);
}

  
                                 
                                                    
   
static struct mbuf *
key_getmsgbuf_x1(
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct mbuf *n;
	int mbufItems[] = {SADB_EXT_RESERVED, SADB_EXT_SA,
		           SADB_X_EXT_SA2, SADB_EXT_ADDRESS_SRC,
		           SADB_EXT_ADDRESS_DST, SADB_EXT_LIFETIME_HARD,
		           SADB_EXT_LIFETIME_SOFT, SADB_EXT_IDENTITY_SRC,
		           SADB_EXT_IDENTITY_DST};

	                  
	if (m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_getmsgbuf_x1: NULL pointer is passed.\n");
	}

	                                   
	n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
	if (!n) {
		return NULL;
	}

	if (n->m_len < sizeof(struct sadb_msg)) {
		n = m_pullup(n, sizeof(struct sadb_msg));
		if (n == NULL) {
			return NULL;
		}
	}
	mtod(n, struct sadb_msg *)->sadb_msg_errno = 0;
	VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
	mtod(n, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

	return n;
}

static int key_delete_all(struct socket *, struct mbuf *,
    const struct sadb_msghdr *, u_int16_t);

  
                         
          
                               
                                             
            
                               
                
  
                          
   
static int
key_delete(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int16_t proto;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_delete: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_delete: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	lck_mtx_lock(sadb_mutex);

	if (mhp->ext[SADB_EXT_SA] == NULL) {
		  
                                                 
                                                 
                         
     
		ipseclog((LOG_DEBUG, "key_delete: doing delete all.\n"));
		                                            
		return key_delete_all(so, m, mhp, proto);
	} else if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa)) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	u_int ipsec_if_index = 0;
	if (ipsec_if != NULL) {
		ipsec_if_index = ipsec_if->if_index;
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                       
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, src0 + 1, dst0 + 1, ipsec_if_index, &saidx);


	                     
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0) {
			continue;
		}

		                        
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		if (sav) {
			break;
		}
	}
	if (sah == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_delete: no SA found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	key_sa_chgstate(sav, SADB_SASTATE_DEAD);
	key_freesav(sav, KEY_SADB_LOCKED);

	lck_mtx_unlock(sadb_mutex);
	sav = NULL;

	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int mbufItems[] = {SADB_EXT_RESERVED, SADB_EXT_SA,
			           SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                                                         
   
static int
key_delete_all(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp,
	u_int16_t proto)
{
	struct sadb_address *src0, *dst0;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav, *nextsav;
	u_int stateidx, state;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	u_int ipsec_if_index = 0;
	if (ipsec_if != NULL) {
		ipsec_if_index = ipsec_if->if_index;
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                       
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, src0 + 1, dst0 + 1, ipsec_if_index, &saidx);

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0) {
			continue;
		}

		                                
		for (stateidx = 0;
		    stateidx < _ARRAYLEN(saorder_state_alive);
		    stateidx++) {
			state = saorder_state_alive[stateidx];
			if (state == SADB_SASTATE_LARVAL) {
				continue;
			}
			for (sav = LIST_FIRST(&sah->savtree[state]);
			    sav != NULL; sav = nextsav) {
				nextsav = LIST_NEXT(sav, chain);
				                  
				if (sav->state != state) {
					ipseclog((LOG_DEBUG, "key_delete_all: "
					    "invalid sav->state "
					    "(queue: %d SA: %d)\n",
					    state, sav->state));
					continue;
				}

				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				key_freesav(sav, KEY_SADB_LOCKED);
			}
		}
	}
	lck_mtx_unlock(sadb_mutex);

	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		int mbufItems[] = {SADB_EXT_RESERVED, SADB_EXT_ADDRESS_SRC,
			           SADB_EXT_ADDRESS_DST};

		                                   
		n = key_gather_mbuf(m, mhp, 1, sizeof(mbufItems) / sizeof(int), mbufItems);
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		if (n->m_len < sizeof(struct sadb_msg)) {
			n = m_pullup(n, sizeof(struct sadb_msg));
			if (n == NULL) {
				return key_senderror(so, m, ENOBUFS);
			}
		}
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(n->m_pkthdr.len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(n->m_pkthdr.len);

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
	}
}

  
                      
          
                               
                                                    
            
                                                                    
                                       
                
  
                          
   
static int
key_get(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int16_t proto;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_get: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_get: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_get: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_get: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	sa0 = (struct sadb_sa *)(void *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	u_int ipsec_if_index = 0;
	if (ipsec_if != NULL) {
		ipsec_if_index = ipsec_if->if_index;
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                       
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, src0 + 1, dst0 + 1, ipsec_if_index, &saidx);

	lck_mtx_lock(sadb_mutex);

	                     
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0) {
			continue;
		}

		                        
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		if (sav) {
			break;
		}
	}
	if (sah == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_get: no SA found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	{
		struct mbuf *n;
		u_int8_t satype;

		                         
		if ((satype = key_proto2satype(sah->saidx.proto)) == 0) {
			lck_mtx_unlock(sadb_mutex);
			ipseclog((LOG_DEBUG, "key_get: there was invalid proto in SAD.\n"));
			return key_senderror(so, m, EINVAL);
		}
		lck_mtx_unlock(sadb_mutex);

		                                   
		n = key_setdumpsa(sav, SADB_GET, satype, mhp->msg->sadb_msg_seq,
		    mhp->msg->sadb_msg_pid);



		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
	}
}

  
                       
                      
                                                   
   
static int
key_getsastatbyspi_one(u_int32_t      spi,
    struct sastat *stat)
{
	struct secashead *sah;
	struct secasvar  *sav = NULL;

	if ((void *)stat == NULL) {
		return -1;
	}

	lck_mtx_lock(sadb_mutex);

	                     
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}

		                        
		sav = key_getsavbyspi(sah, spi);
		if (sav) {
			stat->spi = sav->spi;
			stat->created = (u_int32_t)sav->created;
			if (sav->lft_c) {
				bcopy(sav->lft_c, &stat->lft_c, sizeof(stat->lft_c));
			} else {
				bzero(&stat->lft_c, sizeof(stat->lft_c));
			}
			lck_mtx_unlock(sadb_mutex);
			return 0;
		}
	}

	lck_mtx_unlock(sadb_mutex);

	return -1;
}

  
                                      
                      
                                                                         
   
static int
key_getsastatbyspi(struct sastat *stat_arg,
    u_int32_t      max_stat_arg,
    struct sastat *stat_res,
    u_int64_t      stat_res_size,
    u_int32_t     *max_stat_res)
{
	u_int32_t cur, found = 0;

	if (stat_arg == NULL ||
	    stat_res == NULL ||
	    max_stat_res == NULL) {
		return -1;
	}

	u_int64_t max_stats = stat_res_size / (sizeof(struct sastat));
	max_stats = ((max_stat_arg <= max_stats) ? max_stat_arg : max_stats);

	for (cur = 0; cur < max_stats; cur++) {
		if (key_getsastatbyspi_one(stat_arg[cur].spi,
		    &stat_res[found]) == 0) {
			found++;
		}
	}
	*max_stat_res = found;

	if (found) {
		return 0;
	}
	return -1;
}

                                      
static void
key_getcomb_setlifetime(
	struct sadb_comb *comb)
{
	comb->sadb_comb_soft_allocations = 1;
	comb->sadb_comb_hard_allocations = 1;
	comb->sadb_comb_soft_bytes = 0;
	comb->sadb_comb_hard_bytes = 0;
	comb->sadb_comb_hard_addtime = 86400;              
	comb->sadb_comb_soft_addtime = comb->sadb_comb_soft_addtime * 80 / 100;
	comb->sadb_comb_soft_usetime = 28800;                
	comb->sadb_comb_hard_usetime = comb->sadb_comb_hard_usetime * 80 / 100;
}

#if IPSEC_ESP
  
                                         
                                                          
   
static struct mbuf *
key_getcomb_esp(void)
{
	struct sadb_comb *comb;
	const struct esp_algorithm *algo;
	struct mbuf *result = NULL, *m, *n;
	u_int16_t encmin;
	int off, o;
	int totlen;
	u_int8_t i;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_comb));

	m = NULL;
	for (i = 1; i <= SADB_EALG_MAX; i++) {
		algo = esp_algorithm_lookup(i);
		if (!algo) {
			continue;
		}

		if (algo->keymax < ipsec_esp_keymin) {
			continue;
		}
		if (algo->keymin < ipsec_esp_keymin) {
			encmin = (u_int16_t)ipsec_esp_keymin;
		} else {
			encmin = algo->keymin;
		}

		if (ipsec_esp_auth) {
			m = key_getcomb_ah();
		} else {
#if DIAGNOSTIC
			if (l > MLEN) {
				panic("assumption failed in key_getcomb_esp");
			}
#endif
			MGET(m, M_WAITOK, MT_DATA);
			if (m) {
				M_ALIGN(m, l);
				m->m_len = l;
				m->m_next = NULL;
				bzero(mtod(m, caddr_t), m->m_len);
			}
		}
		if (!m) {
			goto fail;
		}

		totlen = 0;
		for (n = m; n; n = n->m_next) {
			totlen += n->m_len;
		}
#if DIAGNOSTIC
		if (totlen % l) {
			panic("assumption failed in key_getcomb_esp");
		}
#endif

		for (off = 0; off < totlen; off += l) {
			n = m_pulldown(m, off, l, &o);
			if (!n) {
				                        
				goto fail;
			}
			comb = (struct sadb_comb *)
			    (void *)(mtod(n, caddr_t) + o);
			bzero(comb, sizeof(*comb));
			key_getcomb_setlifetime(comb);
			comb->sadb_comb_encrypt = i;
			comb->sadb_comb_encrypt_minbits = encmin;
			comb->sadb_comb_encrypt_maxbits = algo->keymax;
		}

		if (!result) {
			result = m;
		} else {
			m_cat(result, m);
		}
	}

	return result;

fail:
	if (result) {
		m_freem(result);
	}
	return NULL;
}
#endif

  
                                         
   
static struct mbuf *
key_getcomb_ah(void)
{
	struct sadb_comb *comb;
	const struct ah_algorithm *algo;
	struct mbuf *m;
	u_int16_t keymin;
	u_int8_t i;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_comb));

	m = NULL;
	for (i = 1; i <= SADB_AALG_MAX; i++) {
#if 1
		                                                   
		if (i != SADB_AALG_SHA1HMAC && i != SADB_AALG_MD5HMAC) {
			continue;
		}
#endif
		algo = ah_algorithm_lookup(i);
		if (!algo) {
			continue;
		}

		if (algo->keymax < ipsec_ah_keymin) {
			continue;
		}
		if (algo->keymin < ipsec_ah_keymin) {
			keymin = (u_int16_t)ipsec_ah_keymin;
		} else {
			keymin = algo->keymin;
		}

		if (!m) {
#if DIAGNOSTIC
			if (l > MLEN) {
				panic("assumption failed in key_getcomb_ah");
			}
#endif
			MGET(m, M_WAITOK, MT_DATA);
			if (m) {
				M_ALIGN(m, l);
				m->m_len = l;
				m->m_next = NULL;
			}
		} else {
			M_PREPEND(m, l, M_WAITOK, 1);
		}
		if (!m) {
			return NULL;
		}

		comb = mtod(m, struct sadb_comb *);
		bzero(comb, sizeof(*comb));
		key_getcomb_setlifetime(comb);
		comb->sadb_comb_auth = i;
		comb->sadb_comb_auth_minbits = keymin;
		comb->sadb_comb_auth_maxbits = algo->keymax;
	}

	return m;
}

  
                                                         
                       
                                                
   
static struct mbuf *
key_getprop(
	const struct secasindex *saidx)
{
	struct sadb_prop *prop;
	struct mbuf *m, *n;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_prop));
	int totlen;

	switch (saidx->proto) {
#if IPSEC_ESP
	case IPPROTO_ESP:
		m = key_getcomb_esp();
		break;
#endif
	case IPPROTO_AH:
		m = key_getcomb_ah();
		break;
	default:
		return NULL;
	}

	if (!m) {
		return NULL;
	}
	M_PREPEND(m, l, M_WAITOK, 1);
	if (!m) {
		return NULL;
	}

	totlen = 0;
	for (n = m; n; n = n->m_next) {
		totlen += n->m_len;
	}

	prop = mtod(m, struct sadb_prop *);
	bzero(prop, sizeof(*prop));
	VERIFY(totlen <= UINT16_MAX);
	prop->sadb_prop_len = (u_int16_t)PFKEY_UNIT64(totlen);
	prop->sadb_prop_exttype = SADB_EXT_PROPOSAL;
	prop->sadb_prop_replay = 32;             

	return m;
}

  
                                                                           
       
                                                    
                                                 
                                
                                                
     
                                                                   
                      
  
                                                       
                                    
  
       
                     
                          
   
static int
key_acquire(
	struct secasindex *saidx,
	struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
#ifndef IPSEC_NONBLOCK_ACQUIRE
	struct secacq *newacq;
#endif
	u_int8_t satype;
	int error = -1;
	u_int32_t seq;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (saidx == NULL) {
		panic("key_acquire: NULL pointer is passed.\n");
	}
	if ((satype = key_proto2satype(saidx->proto)) == 0) {
		panic("key_acquire: invalid proto is passed.\n");
	}

#ifndef IPSEC_NONBLOCK_ACQUIRE
	  
                                                             
                                                                  
                                                              
                                
    
	                                                          
	lck_mtx_lock(sadb_mutex);
	if ((newacq = key_getacq(saidx)) != NULL) {
		if (key_blockacq_count < newacq->count) {
			                                        
			newacq->count = 0;
		} else {
			                                       
			newacq->count++;
			lck_mtx_unlock(sadb_mutex);
			return 0;
		}
	} else {
		                                                       
		if ((newacq = key_newacq(saidx)) == NULL) {
			lck_mtx_unlock(sadb_mutex);
			return ENOBUFS;
		}

		                    
		LIST_INSERT_HEAD(&acqtree, newacq, chain);
		key_start_timehandler();
	}
	seq = newacq->seq;
	lck_mtx_unlock(sadb_mutex);

#else
	seq = (acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
#endif
	m = key_setsadbmsg(SADB_ACQUIRE, 0, satype, seq, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	                                   
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    (struct sockaddr *)&saidx->src, FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    (struct sockaddr *)&saidx->dst, FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	                                  

	                       
	if (sp) {
		m = key_setsadbxpolicy((u_int16_t)sp->policy, sp->spidx.dir, sp->id);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	}

	                                

	                                           
	m = key_getprop(saidx);
	  
                                                                  
    
	if (m) {
		m_cat(result, m);
	}

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	VERIFY(PFKEY_UNIT64(result->m_pkthdr.len) <= UINT16_MAX);
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

fail:
	if (result) {
		m_freem(result);
	}
	return error;
}

#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *
key_newacq(
	struct secasindex *saidx)
{
	struct secacq *newacq;
	struct timeval tv;

	                   
	KMALLOC_NOWAIT(newacq, struct secacq *, sizeof(struct secacq));
	if (newacq == NULL) {
		lck_mtx_unlock(sadb_mutex);
		KMALLOC_WAIT(newacq, struct secacq *, sizeof(struct secacq));
		lck_mtx_lock(sadb_mutex);
		if (newacq == NULL) {
			ipseclog((LOG_DEBUG, "key_newacq: No more memory.\n"));
			return NULL;
		}
	}
	bzero(newacq, sizeof(*newacq));

	                   
	bcopy(saidx, &newacq->saidx, sizeof(newacq->saidx));
	newacq->seq = (acq_seq == ~0 ? 1 : ++acq_seq);
	microtime(&tv);
	newacq->created = tv.tv_sec;
	newacq->count = 0;

	return newacq;
}

static struct secacq *
key_getacq(
	struct secasindex *saidx)
{
	struct secacq *acq;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	LIST_FOREACH(acq, &acqtree, chain) {
		if (key_cmpsaidx(saidx, &acq->saidx, CMP_EXACTLY)) {
			return acq;
		}
	}

	return NULL;
}

static struct secacq *
key_getacqbyseq(
	u_int32_t seq)
{
	struct secacq *acq;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	LIST_FOREACH(acq, &acqtree, chain) {
		if (acq->seq == seq) {
			return acq;
		}
	}

	return NULL;
}
#endif

static struct secspacq *
key_newspacq(
	struct secpolicyindex *spidx)
{
	struct secspacq *acq;
	struct timeval tv;

	                   
	KMALLOC_NOWAIT(acq, struct secspacq *, sizeof(struct secspacq));
	if (acq == NULL) {
		lck_mtx_unlock(sadb_mutex);
		KMALLOC_WAIT(acq, struct secspacq *, sizeof(struct secspacq));
		lck_mtx_lock(sadb_mutex);
		if (acq == NULL) {
			ipseclog((LOG_DEBUG, "key_newspacq: No more memory.\n"));
			return NULL;
		}
	}
	bzero(acq, sizeof(*acq));

	                   
	bcopy(spidx, &acq->spidx, sizeof(acq->spidx));
	microtime(&tv);
	acq->created = tv.tv_sec;
	acq->count = 0;

	return acq;
}

static struct secspacq *
key_getspacq(
	struct secpolicyindex *spidx)
{
	struct secspacq *acq;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	LIST_FOREACH(acq, &spacqtree, chain) {
		if (key_cmpspidx_exactly(spidx, &acq->spidx)) {
			return acq;
		}
	}

	return NULL;
}

  
                           
                                   
           
                                                            
  
                                    
                                                                               
                                       
                                                                               
                 
  
                          
   
static int
key_acquire2(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	const struct sadb_address *src0, *dst0;
	ifnet_t ipsec_if = NULL;
	struct secasindex saidx;
	struct secashead *sah;
	u_int16_t proto;
	int error;


	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_acquire2: NULL pointer is passed.\n");
	}

	  
                           
                                                                     
                                                       
                                                                  
    
	lck_mtx_lock(sadb_mutex);

	if (mhp->msg->sadb_msg_len == PFKEY_UNIT64(sizeof(struct sadb_msg))) {
#ifndef IPSEC_NONBLOCK_ACQUIRE
		struct secacq *acq;
		struct timeval tv;

		                           
		if (mhp->msg->sadb_msg_seq == 0) {
			lck_mtx_unlock(sadb_mutex);
			ipseclog((LOG_DEBUG, "key_acquire2: must specify sequence number.\n"));
			m_freem(m);
			return 0;
		}

		if ((acq = key_getacqbyseq(mhp->msg->sadb_msg_seq)) == NULL) {
			  
                                                        
                                                          
      
			lck_mtx_unlock(sadb_mutex);
			m_freem(m);
			return 0;
		}

		                                                           
		microtime(&tv);
		acq->created = tv.tv_sec;
		acq->count = 0;
#endif
		lck_mtx_unlock(sadb_mutex);
		m_freem(m);
		return 0;
	}

	  
                                   
    

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_acquire2: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_EXT_PROPOSAL] == NULL) {
		           
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_acquire2: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_PROPOSAL] < sizeof(struct sadb_prop)) {
		           
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_acquire2: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	src0 = (const struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (const struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	ipsec_if = key_get_ipsec_if_from_message(mhp, SADB_X_EXT_IPSECIF);

	u_int ipsec_if_index = 0;
	if (ipsec_if != NULL) {
		ipsec_if_index = ipsec_if->if_index;
		ifnet_release(ipsec_if);
		ipsec_if = NULL;
	}

	                                       
	                   
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, src0 + 1, dst0 + 1, ipsec_if_index, &saidx);

	                    
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD) {
			continue;
		}
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_MODE | CMP_REQID)) {
			break;
		}
	}
	if (sah != NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "key_acquire2: a SA exists already.\n"));
		return key_senderror(so, m, EEXIST);
	}
	lck_mtx_unlock(sadb_mutex);
	error = key_acquire(&saidx, NULL);
	if (error != 0) {
		ipseclog((LOG_DEBUG, "key_acquire2: error %d returned "
		    "from key_acquire.\n", mhp->msg->sadb_msg_errno));
		return key_senderror(so, m, error);
	}

	return key_sendup_mbuf(so, m, KEY_SENDUP_REGISTERED);
}

  
                            
                                                                          
          
           
                                                                 
           
                      
                    
                                                 
  
                          
   
static int
key_register(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct secreg *reg, *newreg = 0;

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_register: NULL pointer is passed.\n");
	}

	                                        
	if (mhp->msg->sadb_msg_satype >= sizeof(regtree) / sizeof(regtree[0])) {
		return key_senderror(so, m, EINVAL);
	}

	                                                                  
	if (mhp->msg->sadb_msg_satype == SADB_SATYPE_UNSPEC) {
		goto setmsg;
	}

	                    
	KMALLOC_WAIT(newreg, struct secreg *, sizeof(*newreg));
	if (newreg == NULL) {
		ipseclog((LOG_DEBUG, "key_register: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}
	bzero((caddr_t)newreg, sizeof(*newreg));

	lck_mtx_lock(sadb_mutex);
	                                   
	LIST_FOREACH(reg, &regtree[mhp->msg->sadb_msg_satype], chain) {
		if (reg->so == so) {
			lck_mtx_unlock(sadb_mutex);
			ipseclog((LOG_DEBUG, "key_register: socket exists already.\n"));
			KFREE(newreg);
			return key_senderror(so, m, EEXIST);
		}
	}

	socket_lock(so, 1);
	newreg->so = so;
	((struct keycb *)sotorawcb(so))->kp_registered++;
	socket_unlock(so, 1);

	                             
	LIST_INSERT_HEAD(&regtree[mhp->msg->sadb_msg_satype], newreg, chain);
	lck_mtx_unlock(sadb_mutex);
setmsg:
	{
		struct mbuf *n;
		struct sadb_msg *newmsg;
		struct sadb_supported *sup;
		u_int16_t len, alen, elen;
		int off;
		u_int8_t i;
		struct sadb_alg *alg;

		                                   
		alen = 0;
		for (i = 1; i <= SADB_AALG_MAX; i++) {
			if (ah_algorithm_lookup(i)) {
				alen += sizeof(struct sadb_alg);
			}
		}
		if (alen) {
			alen += sizeof(struct sadb_supported);
		}
		elen = 0;
#if IPSEC_ESP
		for (i = 1; i <= SADB_EALG_MAX; i++) {
			if (esp_algorithm_lookup(i)) {
				elen += sizeof(struct sadb_alg);
			}
		}
		if (elen) {
			elen += sizeof(struct sadb_supported);
		}
#endif

		len = sizeof(struct sadb_msg) + alen + elen;

		if (len > MCLBYTES) {
			return key_senderror(so, m, ENOBUFS);
		}

		MGETHDR(n, M_WAITOK, MT_DATA);
		if (n && len > MHLEN) {
			MCLGET(n, M_WAITOK);
			if ((n->m_flags & M_EXT) == 0) {
				m_freem(n);
				n = NULL;
			}
		}
		if (!n) {
			return key_senderror(so, m, ENOBUFS);
		}

		n->m_pkthdr.len = n->m_len = len;
		n->m_next = NULL;
		off = 0;

		m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, caddr_t) + off);
		newmsg = mtod(n, struct sadb_msg *);
		newmsg->sadb_msg_errno = 0;
		VERIFY(PFKEY_UNIT64(len) <= UINT16_MAX);
		newmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(len);
		off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

		                                  
		if (alen) {
			sup = (struct sadb_supported *)(void *)(mtod(n, caddr_t) + off);
			sup->sadb_supported_len = (u_int16_t)PFKEY_UNIT64(alen);
			sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_AUTH;
			off += PFKEY_ALIGN8(sizeof(*sup));

			for (i = 1; i <= SADB_AALG_MAX; i++) {
				const struct ah_algorithm *aalgo;

				aalgo = ah_algorithm_lookup(i);
				if (!aalgo) {
					continue;
				}
				alg = (struct sadb_alg *)
				    (void *)(mtod(n, caddr_t) + off);
				alg->sadb_alg_id = i;
				alg->sadb_alg_ivlen = 0;
				alg->sadb_alg_minbits = aalgo->keymin;
				alg->sadb_alg_maxbits = aalgo->keymax;
				off += PFKEY_ALIGN8(sizeof(*alg));
			}
		}

#if IPSEC_ESP
		                              
		if (elen) {
			sup = (struct sadb_supported *)(void *)(mtod(n, caddr_t) + off);
			sup->sadb_supported_len = PFKEY_UNIT64(elen);
			sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_ENCRYPT;
			off += PFKEY_ALIGN8(sizeof(*sup));

			for (i = 1; i <= SADB_EALG_MAX; i++) {
				const struct esp_algorithm *ealgo;

				ealgo = esp_algorithm_lookup(i);
				if (!ealgo) {
					continue;
				}
				alg = (struct sadb_alg *)
				    (void *)(mtod(n, caddr_t) + off);
				alg->sadb_alg_id = i;
				if (ealgo && ealgo->ivlen) {
					  
                                               
                                        
        
					VERIFY((*ealgo->ivlen)(ealgo, NULL) <= UINT8_MAX);
					alg->sadb_alg_ivlen =
					    (u_int8_t)((*ealgo->ivlen)(ealgo, NULL));
				} else {
					alg->sadb_alg_ivlen = 0;
				}
				alg->sadb_alg_minbits = ealgo->keymin;
				alg->sadb_alg_maxbits = ealgo->keymax;
				off += PFKEY_ALIGN8(sizeof(struct sadb_alg));
			}
		}
#endif

#if DIAGNOSTIC
		if (off != len) {
			panic("length assumption failed in key_register");
		}
#endif

		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_REGISTERED);
	}
}

static void
key_delete_all_for_socket(struct socket *so)
{
	struct secashead *sah, *nextsah;
	struct secasvar *sav, *nextsav;
	u_int stateidx;
	u_int state;

	for (sah = LIST_FIRST(&sahtree);
	    sah != NULL;
	    sah = nextsah) {
		nextsah = LIST_NEXT(sah, chain);
		for (stateidx = 0; stateidx < _ARRAYLEN(saorder_state_alive); stateidx++) {
			state = saorder_state_any[stateidx];
			for (sav = LIST_FIRST(&sah->savtree[state]); sav != NULL; sav = nextsav) {
				nextsav = LIST_NEXT(sav, chain);
				if (sav->flags2 & SADB_X_EXT_SA2_DELETE_ON_DETACH &&
				    sav->so == so) {
					key_sa_chgstate(sav, SADB_SASTATE_DEAD);
					key_freesav(sav, KEY_SADB_LOCKED);
				}
			}
		}
	}
}

  
                                
                                                                      
   
void
key_freereg(
	struct socket *so)
{
	struct secreg *reg;
	int i;

	                  
	if (so == NULL) {
		panic("key_freereg: NULL pointer is passed.\n");
	}

	  
                                  
                                                           
                                                    
    
	lck_mtx_lock(sadb_mutex);
	key_delete_all_for_socket(so);
	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		LIST_FOREACH(reg, &regtree[i], chain) {
			if (reg->so == so
			    && __LIST_CHAINED(reg)) {
				LIST_REMOVE(reg, chain);
				KFREE(reg);
				break;
			}
		}
	}
	lck_mtx_unlock(sadb_mutex);
	return;
}

  
                         
       
                                                            
                    
                                              
  
                   
                        
   
static int
key_expire(
	struct secasvar *sav)
{
	u_int8_t satype;
	struct mbuf *result = NULL, *m;
	int len;
	int error = -1;
	struct sadb_lifetime *lt;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (sav == NULL) {
		panic("key_expire: NULL pointer is passed.\n");
	}
	if (sav->sah == NULL) {
		panic("key_expire: Why was SA index in SA NULL.\n");
	}
	if ((satype = key_proto2satype(sav->sah->saidx.proto)) == 0) {
		panic("key_expire: invalid proto is passed.\n");
	}

	                    
	m = key_setsadbmsg(SADB_EXPIRE, 0, satype, sav->seq, 0, (u_int16_t)sav->refcnt);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	                         
	m = key_setsadbsa(sav);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	                         
	m = key_setsadbxsa2(sav->sah->saidx.mode,
	    sav->replay[0] ? sav->replay[0]->count : 0,
	    sav->sah->saidx.reqid,
	    sav->flags2);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	                                                  
	len = PFKEY_ALIGN8(sizeof(*lt)) * 2;
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {         
		if (m) {
			m_freem(m);
		}
		error = ENOBUFS;
		goto fail;
	}
	bzero(mtod(m, caddr_t), len);
	lt = mtod(m, struct sadb_lifetime *);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lt->sadb_lifetime_allocations = sav->lft_c->sadb_lifetime_allocations;
	lt->sadb_lifetime_bytes = sav->lft_c->sadb_lifetime_bytes;
	lt->sadb_lifetime_addtime = sav->lft_c->sadb_lifetime_addtime;
	lt->sadb_lifetime_usetime = sav->lft_c->sadb_lifetime_usetime;
	lt = (struct sadb_lifetime *)(void *)(mtod(m, caddr_t) + len / 2);
	bcopy(sav->lft_s, lt, sizeof(*lt));
	m_cat(result, m);

	                                 
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    (struct sockaddr *)&sav->sah->saidx.src,
	    FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	                                      
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    (struct sockaddr *)&sav->sah->saidx.dst,
	    FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	VERIFY(PFKEY_UNIT64(result->m_pkthdr.len) <= UINT16_MAX);
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

fail:
	if (result) {
		m_freem(result);
	}
	return error;
}

  
                        
          
           
                                                     
            
           
                
                                                 
  
                          
   
static int
key_flush(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secashead *sah, *nextsah;
	struct secasvar *sav, *nextsav;
	u_int16_t proto;
	u_int state;
	u_int stateidx;

	                  
	if (so == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_flush: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_flush: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	lck_mtx_lock(sadb_mutex);

	                                                
	for (sah = LIST_FIRST(&sahtree);
	    sah != NULL;
	    sah = nextsah) {
		nextsah = LIST_NEXT(sah, chain);

		if (mhp->msg->sadb_msg_satype != SADB_SATYPE_UNSPEC
		    && proto != sah->saidx.proto) {
			continue;
		}

		for (stateidx = 0;
		    stateidx < _ARRAYLEN(saorder_state_alive);
		    stateidx++) {
			state = saorder_state_any[stateidx];
			for (sav = LIST_FIRST(&sah->savtree[state]);
			    sav != NULL;
			    sav = nextsav) {
				nextsav = LIST_NEXT(sav, chain);

				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				key_freesav(sav, KEY_SADB_LOCKED);
			}
		}

		sah->state = SADB_SASTATE_DEAD;
	}
	lck_mtx_unlock(sadb_mutex);

	if (m->m_len < sizeof(struct sadb_msg) ||
	    sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		ipseclog((LOG_DEBUG, "key_flush: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	if (m->m_next) {
		m_freem(m->m_next);
	}
	m->m_next = NULL;
	m->m_pkthdr.len = m->m_len = sizeof(struct sadb_msg);
	newmsg = mtod(m, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	VERIFY(PFKEY_UNIT64(m->m_pkthdr.len) <= UINT16_MAX);
	newmsg->sadb_msg_len = (uint16_t)PFKEY_UNIT64(m->m_pkthdr.len);

	return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
}

  
                       
                                                    
          
           
                                               
            
                 
                
  
                          
   

struct sav_dump_elem {
	struct secasvar *sav;
	u_int8_t satype;
};

static int
key_dump(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct secashead *sah;
	struct secasvar *sav;
	struct sav_dump_elem *savbuf = NULL, *elem_ptr;
	size_t total_req_size = 0;
	u_int32_t bufcount = 0, cnt = 0, cnt2 = 0;
	u_int16_t proto;
	u_int stateidx;
	u_int8_t satype;
	u_int state;
	struct mbuf *n;
	int error = 0;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_dump: NULL pointer is passed.\n");
	}

	                         
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_dump: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if ((bufcount = ipsec_sav_count) == 0) {
		error = ENOENT;
		goto end;
	}

	if (os_add_overflow(bufcount, 512, &bufcount)) {
		ipseclog((LOG_DEBUG, "key_dump: bufcount overflow, ipsec sa count %u.\n", ipsec_sav_count));
		bufcount = ipsec_sav_count;
	}

	if (os_mul_overflow(bufcount, sizeof(struct sav_dump_elem), &total_req_size)) {
		panic("key_dump savbuf requested memory overflow %u\n", bufcount);
	}

	KMALLOC_WAIT(savbuf, struct sav_dump_elem*, total_req_size);
	if (savbuf == NULL) {
		ipseclog((LOG_DEBUG, "key_dump: No more memory.\n"));
		error = ENOMEM;
		goto end;
	}

	                                                   
	lck_mtx_lock(sadb_mutex);
	elem_ptr = savbuf;
	LIST_FOREACH(sah, &sahtree, chain) {
		if (mhp->msg->sadb_msg_satype != SADB_SATYPE_UNSPEC
		    && proto != sah->saidx.proto) {
			continue;
		}

		                         
		if ((satype = key_proto2satype(sah->saidx.proto)) == 0) {
			lck_mtx_unlock(sadb_mutex);
			ipseclog((LOG_DEBUG, "key_dump: there was invalid proto in SAD.\n"));
			error = EINVAL;
			goto end;
		}

		for (stateidx = 0;
		    stateidx < _ARRAYLEN(saorder_state_any);
		    stateidx++) {
			state = saorder_state_any[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				if (cnt == bufcount) {
					break;                                   
				}
				elem_ptr->sav = sav;
				elem_ptr->satype = satype;
				sav->refcnt++;
				elem_ptr++;
				cnt++;
			}
		}
	}
	lck_mtx_unlock(sadb_mutex);

	if (cnt == 0) {
		error = ENOENT;
		goto end;
	}

	                                               
	elem_ptr = savbuf;
	cnt2 = cnt;
	while (cnt2) {
		n = key_setdumpsa(elem_ptr->sav, SADB_DUMP, elem_ptr->satype,
		    --cnt2, mhp->msg->sadb_msg_pid);

		if (!n) {
			error = ENOBUFS;
			goto end;
		}

		key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
		elem_ptr++;
	}

end:
	if (savbuf) {
		if (cnt) {
			elem_ptr = savbuf;
			lck_mtx_lock(sadb_mutex);
			while (cnt--) {
				key_freesav((elem_ptr++)->sav, KEY_SADB_LOCKED);
			}
			lck_mtx_unlock(sadb_mutex);
		}
		KFREE(savbuf);
	}

	if (error) {
		return key_senderror(so, m, error);
	}

	m_freem(m);
	return 0;
}

  
                            
  
                          
   
static int
key_promisc(
	struct socket *so,
	struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	int olen;

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("key_promisc: NULL pointer is passed.\n");
	}

	olen = PFKEY_UNUNIT64(mhp->msg->sadb_msg_len);

	if (olen < sizeof(struct sadb_msg)) {
#if 1
		return key_senderror(so, m, EINVAL);
#else
		m_freem(m);
		return 0;
#endif
	} else if (olen == sizeof(struct sadb_msg)) {
		                                 
		struct keycb *kp;

		socket_lock(so, 1);
		if ((kp = (struct keycb *)sotorawcb(so)) == NULL) {
			return key_senderror(so, m, EINVAL);
		}
		mhp->msg->sadb_msg_errno = 0;
		switch (mhp->msg->sadb_msg_satype) {
		case 0:
		case 1:
			kp->kp_promisc = mhp->msg->sadb_msg_satype;
			break;
		default:
			socket_unlock(so, 1);
			return key_senderror(so, m, EINVAL);
		}
		socket_unlock(so, 1);

		                                                
		mhp->msg->sadb_msg_errno = 0;
		return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
	} else {
		                       

		m_adj(m, PFKEY_ALIGN8(sizeof(struct sadb_msg)));

		                                                              
		return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
	}
}

static int(*const key_typesw[])(struct socket *, struct mbuf *,
    const struct sadb_msghdr *) = {
	NULL,                              
	key_getspi,                      
	key_update,                      
	key_add,                      
	key_delete,                      
	key_get,                      
	key_acquire2,                     
	key_register,                      
	NULL,                            
	key_flush,                      
	key_dump,                      
	key_promisc,                        
	NULL,                               
	key_spdadd,                           
	key_spdadd,                        
	key_spddelete,                        
	key_spdget,                        
	NULL,                                  
	key_spddump,                        
	key_spdflush,                        
	key_spdadd,                           
	NULL,                                 
	key_spddelete2,                        
	key_getsastat,                       
	key_spdenable,                         
	key_spddisable,                          
	key_migrate,                     
};

static void
bzero_mbuf(struct mbuf *m)
{
	struct mbuf *mptr  = m;
	struct sadb_msg *msg = NULL;
	int offset = 0;

	if (!mptr) {
		return;
	}

	if (mptr->m_len >= sizeof(struct sadb_msg)) {
		msg = mtod(mptr, struct sadb_msg *);
		if (msg->sadb_msg_type != SADB_ADD &&
		    msg->sadb_msg_type != SADB_UPDATE) {
			return;
		}
		offset = sizeof(struct sadb_msg);
	}
	bzero(mptr->m_data + offset, mptr->m_len - offset);
	mptr = mptr->m_next;
	while (mptr != NULL) {
		bzero(mptr->m_data, mptr->m_len);
		mptr = mptr->m_next;
	}
}

static void
bzero_keys(const struct sadb_msghdr *mh)
{
	int extlen = 0;
	int offset = 0;

	if (!mh) {
		return;
	}
	offset = sizeof(struct sadb_key);

	if (mh->ext[SADB_EXT_KEY_ENCRYPT]) {
		struct sadb_key *key = (struct sadb_key*)mh->ext[SADB_EXT_KEY_ENCRYPT];
		extlen = key->sadb_key_bits >> 3;

		if (mh->extlen[SADB_EXT_KEY_ENCRYPT] >= offset + extlen) {
			bzero((uint8_t *)mh->ext[SADB_EXT_KEY_ENCRYPT] + offset, extlen);
		} else {
			bzero(mh->ext[SADB_EXT_KEY_ENCRYPT], mh->extlen[SADB_EXT_KEY_ENCRYPT]);
		}
	}
	if (mh->ext[SADB_EXT_KEY_AUTH]) {
		struct sadb_key *key = (struct sadb_key*)mh->ext[SADB_EXT_KEY_AUTH];
		extlen = key->sadb_key_bits >> 3;

		if (mh->extlen[SADB_EXT_KEY_AUTH] >= offset + extlen) {
			bzero((uint8_t *)mh->ext[SADB_EXT_KEY_AUTH] + offset, extlen);
		} else {
			bzero(mh->ext[SADB_EXT_KEY_AUTH], mh->extlen[SADB_EXT_KEY_AUTH]);
		}
	}
}

static int
key_validate_address_pair(struct sadb_address *src0,
    struct sadb_address *dst0)
{
	u_int plen = 0;

	                                
	if (src0->sadb_address_proto != dst0->sadb_address_proto) {
		ipseclog((LOG_DEBUG, "key_parse: upper layer protocol mismatched.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
		return EINVAL;
	}

	                  
	if (PFKEY_ADDR_SADDR(src0)->sa_family !=
	    PFKEY_ADDR_SADDR(dst0)->sa_family) {
		ipseclog((LOG_DEBUG, "key_parse: address family mismatched.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
		return EINVAL;
	}
	if (PFKEY_ADDR_SADDR(src0)->sa_len !=
	    PFKEY_ADDR_SADDR(dst0)->sa_len) {
		ipseclog((LOG_DEBUG,
		    "key_parse: address struct size mismatched.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
		return EINVAL;
	}

	switch (PFKEY_ADDR_SADDR(src0)->sa_family) {
	case AF_INET:
		if (PFKEY_ADDR_SADDR(src0)->sa_len != sizeof(struct sockaddr_in)) {
			PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
			return EINVAL;
		}
		break;
	case AF_INET6:
		if (PFKEY_ADDR_SADDR(src0)->sa_len != sizeof(struct sockaddr_in6)) {
			PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
			return EINVAL;
		}
		break;
	default:
		ipseclog((LOG_DEBUG,
		    "key_parse: unsupported address family.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
		return EAFNOSUPPORT;
	}

	switch (PFKEY_ADDR_SADDR(src0)->sa_family) {
	case AF_INET:
		plen = sizeof(struct in_addr) << 3;
		break;
	case AF_INET6:
		plen = sizeof(struct in6_addr) << 3;
		break;
	default:
		plen = 0;                           
		break;
	}

	                             
	if (src0->sadb_address_prefixlen > plen ||
	    dst0->sadb_address_prefixlen > plen) {
		ipseclog((LOG_DEBUG,
		    "key_parse: illegal prefixlen.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invaddr);
		return EINVAL;
	}

	  
                                                            
                              
    
	return 0;
}

  
                                            
                                           
                                           
      
                                                                
                                            
                                 
       
                                                
   
int
key_parse(
	struct mbuf *m,
	struct socket *so)
{
	struct sadb_msg *msg;
	struct sadb_msghdr mh;
	u_int orglen;
	int error;
	int target;
	Boolean keyAligned = FALSE;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                  
	if (m == NULL || so == NULL) {
		panic("key_parse: NULL pointer is passed.\n");
	}

#if 0                                               
	KEYDEBUG(KEYDEBUG_KEY_DUMP,
	    ipseclog((LOG_DEBUG, "key_parse: passed sadb_msg\n"));
	    kdebug_sadb(msg));
#endif

	if (m->m_len < sizeof(struct sadb_msg)) {
		m = m_pullup(m, sizeof(struct sadb_msg));
		if (!m) {
			return ENOBUFS;
		}
	}
	msg = mtod(m, struct sadb_msg *);
	orglen = PFKEY_UNUNIT64(msg->sadb_msg_len);
	target = KEY_SENDUP_ONE;

	if ((m->m_flags & M_PKTHDR) == 0 ||
	    m->m_pkthdr.len != orglen) {
		ipseclog((LOG_DEBUG, "key_parse: invalid message length.\n"));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invlen);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_version != PF_KEY_V2) {
		ipseclog((LOG_DEBUG,
		    "key_parse: PF_KEY version %u is mismatched.\n",
		    msg->sadb_msg_version));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invver);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_type > SADB_MAX) {
		ipseclog((LOG_DEBUG, "key_parse: invalid type %u is passed.\n",
		    msg->sadb_msg_type));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invmsgtype);
		error = EINVAL;
		goto senderror;
	}

	                                              
	if (m->m_pkthdr.len > MCLBYTES) {
		m_freem(m);
		return ENOBUFS;
	}
	if (m->m_next) {
		struct mbuf *n;

		MGETHDR(n, M_WAITOK, MT_DATA);
		if (n && m->m_pkthdr.len > MHLEN) {
			MCLGET(n, M_WAITOK);
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				n = NULL;
			}
		}
		if (!n) {
			bzero_mbuf(m);
			m_freem(m);
			return ENOBUFS;
		}
		m_copydata(m, 0, m->m_pkthdr.len, mtod(n, caddr_t));
		n->m_pkthdr.len = n->m_len = m->m_pkthdr.len;
		n->m_next = NULL;
		bzero_mbuf(m);
		m_freem(m);
		m = n;
	}

	                                                                       
	error = key_align(m, &mh);
	if (error) {
		return error;
	}

	if (m->m_next) {               
		bzero_mbuf(m);
		m_freem(m);
		return ENOBUFS;
	}

	keyAligned = TRUE;
	msg = mh.msg;

	                   
	switch (msg->sadb_msg_satype) {
	case SADB_SATYPE_UNSPEC:
		switch (msg->sadb_msg_type) {
		case SADB_GETSPI:
		case SADB_UPDATE:
		case SADB_ADD:
		case SADB_DELETE:
		case SADB_GET:
		case SADB_ACQUIRE:
		case SADB_EXPIRE:
			ipseclog((LOG_DEBUG, "key_parse: must specify satype "
			    "when msg type=%u.\n", msg->sadb_msg_type));
			PFKEY_STAT_INCREMENT(pfkeystat.out_invsatype);
			error = EINVAL;
			goto senderror;
		}
		break;
	case SADB_SATYPE_AH:
	case SADB_SATYPE_ESP:
		switch (msg->sadb_msg_type) {
		case SADB_X_SPDADD:
		case SADB_X_SPDDELETE:
		case SADB_X_SPDGET:
		case SADB_X_SPDDUMP:
		case SADB_X_SPDFLUSH:
		case SADB_X_SPDSETIDX:
		case SADB_X_SPDUPDATE:
		case SADB_X_SPDDELETE2:
		case SADB_X_SPDENABLE:
		case SADB_X_SPDDISABLE:
			ipseclog((LOG_DEBUG, "key_parse: illegal satype=%u\n",
			    msg->sadb_msg_type));
			PFKEY_STAT_INCREMENT(pfkeystat.out_invsatype);
			error = EINVAL;
			goto senderror;
		}
		break;
	case SADB_SATYPE_RSVP:
	case SADB_SATYPE_OSPFV2:
	case SADB_SATYPE_RIPV2:
	case SADB_SATYPE_MIP:
		ipseclog((LOG_DEBUG, "key_parse: type %u isn't supported.\n",
		    msg->sadb_msg_satype));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invsatype);
		error = EOPNOTSUPP;
		goto senderror;
	case 1:                                    
		if (msg->sadb_msg_type == SADB_X_PROMISC) {
			break;
		}
		OS_FALLTHROUGH;
	default:
		ipseclog((LOG_DEBUG, "key_parse: invalid type %u is passed.\n",
		    msg->sadb_msg_satype));
		PFKEY_STAT_INCREMENT(pfkeystat.out_invsatype);
		error = EINVAL;
		goto senderror;
	}

	                                                                  
	void *src0 = mh.ext[SADB_EXT_ADDRESS_SRC];
	void *dst0 = mh.ext[SADB_EXT_ADDRESS_DST];
	if (mh.ext[SADB_X_EXT_ADDR_RANGE_SRC_START] != NULL &&
	    mh.ext[SADB_X_EXT_ADDR_RANGE_SRC_END] != NULL) {
		error = key_validate_address_pair((struct sadb_address *)(mh.ext[SADB_X_EXT_ADDR_RANGE_SRC_START]),
		    (struct sadb_address *)(mh.ext[SADB_X_EXT_ADDR_RANGE_SRC_END]));
		if (error != 0) {
			goto senderror;
		}

		if (src0 == NULL) {
			src0 = mh.ext[SADB_X_EXT_ADDR_RANGE_SRC_START];
		}
	}
	if (mh.ext[SADB_X_EXT_ADDR_RANGE_DST_START] != NULL &&
	    mh.ext[SADB_X_EXT_ADDR_RANGE_DST_END] != NULL) {
		error = key_validate_address_pair((struct sadb_address *)(mh.ext[SADB_X_EXT_ADDR_RANGE_DST_START]),
		    (struct sadb_address *)(mh.ext[SADB_X_EXT_ADDR_RANGE_DST_END]));
		if (error != 0) {
			goto senderror;
		}

		if (dst0 == NULL) {
			dst0 = mh.ext[SADB_X_EXT_ADDR_RANGE_DST_START];
		}
	}
	if (src0 != NULL && dst0 != NULL) {
		error = key_validate_address_pair((struct sadb_address *)(src0),
		    (struct sadb_address *)(dst0));
		if (error != 0) {
			goto senderror;
		}
	}

	void *migrate_src = mh.ext[SADB_EXT_MIGRATE_ADDRESS_SRC];
	void *migrate_dst = mh.ext[SADB_EXT_MIGRATE_ADDRESS_DST];
	if (migrate_src != NULL && migrate_dst != NULL) {
		error = key_validate_address_pair((struct sadb_address *)(migrate_src),
		    (struct sadb_address *)(migrate_dst));
		if (error != 0) {
			goto senderror;
		}
	}

	if (msg->sadb_msg_type >= sizeof(key_typesw) / sizeof(key_typesw[0]) ||
	    key_typesw[msg->sadb_msg_type] == NULL) {
		PFKEY_STAT_INCREMENT(pfkeystat.out_invmsgtype);
		error = EINVAL;
		goto senderror;
	}

	error = (*key_typesw[msg->sadb_msg_type])(so, m, &mh);

	return error;

senderror:
	if (keyAligned) {
		bzero_keys(&mh);
	} else {
		bzero_mbuf(m);
	}
	msg->sadb_msg_errno = (u_int8_t)error;
	return key_sendup_mbuf(so, m, target);
}

static int
key_senderror(
	struct socket *so,
	struct mbuf *m,
	int code)
{
	struct sadb_msg *msg;

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	if (m->m_len < sizeof(struct sadb_msg)) {
		panic("invalid mbuf passed to key_senderror");
	}

	msg = mtod(m, struct sadb_msg *);
	msg->sadb_msg_errno = (u_int8_t)code;
	return key_sendup_mbuf(so, m, KEY_SENDUP_ONE);
}

  
                                                      
                            
                                      
   
static int
key_align(
	struct mbuf *m,
	struct sadb_msghdr *mhp)
{
	struct mbuf *n;
	struct sadb_ext *ext;
	size_t end;
	int off, extlen;
	int toff;

	                  
	if (m == NULL || mhp == NULL) {
		panic("key_align: NULL pointer is passed.\n");
	}
	if (m->m_len < sizeof(struct sadb_msg)) {
		panic("invalid mbuf passed to key_align");
	}

	                
	bzero(mhp, sizeof(*mhp));

	mhp->msg = mtod(m, struct sadb_msg *);
	mhp->ext[0] = (struct sadb_ext *)mhp->msg;                              

	end = PFKEY_UNUNIT64(mhp->msg->sadb_msg_len);
	extlen = (int)end;                                         
	for (off = sizeof(struct sadb_msg); off < end; off += extlen) {
		n = m_pulldown(m, off, sizeof(struct sadb_ext), &toff);
		if (!n) {
			                        
			return ENOBUFS;
		}
		ext = (struct sadb_ext *)(void *)(mtod(n, caddr_t) + toff);

		                 
		switch (ext->sadb_ext_type) {
		case SADB_EXT_SA:
		case SADB_EXT_ADDRESS_SRC:
		case SADB_EXT_ADDRESS_DST:
		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_LIFETIME_CURRENT:
		case SADB_EXT_LIFETIME_HARD:
		case SADB_EXT_LIFETIME_SOFT:
		case SADB_EXT_KEY_AUTH:
		case SADB_EXT_KEY_ENCRYPT:
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		case SADB_EXT_SENSITIVITY:
		case SADB_EXT_PROPOSAL:
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
		case SADB_EXT_SPIRANGE:
		case SADB_X_EXT_POLICY:
		case SADB_X_EXT_SA2:
		case SADB_EXT_SESSION_ID:
		case SADB_EXT_SASTAT:
		case SADB_X_EXT_IPSECIF:
		case SADB_X_EXT_ADDR_RANGE_SRC_START:
		case SADB_X_EXT_ADDR_RANGE_SRC_END:
		case SADB_X_EXT_ADDR_RANGE_DST_START:
		case SADB_X_EXT_ADDR_RANGE_DST_END:
		case SADB_EXT_MIGRATE_ADDRESS_SRC:
		case SADB_EXT_MIGRATE_ADDRESS_DST:
		case SADB_X_EXT_MIGRATE_IPSECIF:
			                     
			  
                                                  
                               
      
			if (mhp->ext[ext->sadb_ext_type] != NULL) {
				ipseclog((LOG_DEBUG,
				    "key_align: duplicate ext_type %u "
				    "is passed.\n", ext->sadb_ext_type));
				bzero_mbuf(m);
				m_freem(m);
				PFKEY_STAT_INCREMENT(pfkeystat.out_dupext);
				return EINVAL;
			}
			break;
		default:
			ipseclog((LOG_DEBUG,
			    "key_align: invalid ext_type %u is passed.\n",
			    ext->sadb_ext_type));
			bzero_mbuf(m);
			m_freem(m);
			PFKEY_STAT_INCREMENT(pfkeystat.out_invexttype);
			return EINVAL;
		}

		extlen = PFKEY_UNUNIT64(ext->sadb_ext_len);
		if (off + extlen > end) {
			ipseclog((LOG_DEBUG,
			    "key_align: ext type %u invalid ext length %d "
			    "offset %d sadb message total len %zu is passed.\n",
			    ext->sadb_ext_type, extlen, off, end));
			bzero_mbuf(m);
			m_freem(m);
			PFKEY_STAT_INCREMENT(pfkeystat.out_invlen);
			return EINVAL;
		}

		if (key_validate_ext(ext, extlen)) {
			bzero_mbuf(m);
			m_freem(m);
			PFKEY_STAT_INCREMENT(pfkeystat.out_invlen);
			return EINVAL;
		}

		n = m_pulldown(m, off, extlen, &toff);
		if (!n) {
			                        
			return ENOBUFS;
		}
		ext = (struct sadb_ext *)(void *)(mtod(n, caddr_t) + toff);

		mhp->ext[ext->sadb_ext_type] = ext;
		mhp->extoff[ext->sadb_ext_type] = off;
		mhp->extlen[ext->sadb_ext_type] = extlen;
	}

	if (off != end) {
		bzero_mbuf(m);
		m_freem(m);
		PFKEY_STAT_INCREMENT(pfkeystat.out_invlen);
		return EINVAL;
	}

	return 0;
}

static int
key_validate_ext(
	const struct sadb_ext *ext,
	int len)
{
	struct sockaddr *sa;
	enum { NONE, ADDR } checktype = NONE;
	int baselen = 0;
	const int sal = offsetof(struct sockaddr, sa_len) + sizeof(sa->sa_len);

	if (len != PFKEY_UNUNIT64(ext->sadb_ext_len)) {
		return EINVAL;
	}

	                                                       
	if (ext->sadb_ext_type >= sizeof(minsize) / sizeof(minsize[0]) ||
	    ext->sadb_ext_type >= sizeof(maxsize) / sizeof(maxsize[0])) {
		return EINVAL;
	}
	if (!minsize[ext->sadb_ext_type] || len < minsize[ext->sadb_ext_type]) {
		return EINVAL;
	}
	if (maxsize[ext->sadb_ext_type] && len > maxsize[ext->sadb_ext_type]) {
		return EINVAL;
	}

	                                                      
	switch (ext->sadb_ext_type) {
	case SADB_EXT_ADDRESS_SRC:
	case SADB_EXT_ADDRESS_DST:
	case SADB_EXT_ADDRESS_PROXY:
	case SADB_X_EXT_ADDR_RANGE_SRC_START:
	case SADB_X_EXT_ADDR_RANGE_SRC_END:
	case SADB_X_EXT_ADDR_RANGE_DST_START:
	case SADB_X_EXT_ADDR_RANGE_DST_END:
	case SADB_EXT_MIGRATE_ADDRESS_SRC:
	case SADB_EXT_MIGRATE_ADDRESS_DST:
		baselen = PFKEY_ALIGN8(sizeof(struct sadb_address));
		checktype = ADDR;
		break;
	case SADB_EXT_IDENTITY_SRC:
	case SADB_EXT_IDENTITY_DST:
		if (((struct sadb_ident *)(uintptr_t)(size_t)ext)->
		    sadb_ident_type == SADB_X_IDENTTYPE_ADDR) {
			baselen = PFKEY_ALIGN8(sizeof(struct sadb_ident));
			checktype = ADDR;
		} else {
			checktype = NONE;
		}
		break;
	default:
		checktype = NONE;
		break;
	}

	switch (checktype) {
	case NONE:
		break;
	case ADDR:
		sa = (struct sockaddr *)((caddr_t)(uintptr_t)ext + baselen);

		if (len < baselen + sal) {
			return EINVAL;
		}
		if (baselen + PFKEY_ALIGN8(sa->sa_len) != len) {
			return EINVAL;
		}
		break;
	}

	                           
	if (ext->sadb_ext_type == SADB_EXT_KEY_AUTH ||
	    ext->sadb_ext_type == SADB_EXT_KEY_ENCRYPT) {
		struct sadb_key *key = (struct sadb_key *)(uintptr_t)ext;
		if (len < (sizeof(struct sadb_key) + _KEYLEN(key))) {
			return EINVAL;
		}
	}

	return 0;
}

  
                                                                     
  
                                         
                                                                              
  
                                 
   
int
key_checktunnelsanity(
	struct secasvar *sav,
	__unused u_int family,
	__unused caddr_t src,
	__unused caddr_t dst)
{
	                  
	if (sav->sah == NULL) {
		panic("sav->sah == NULL at key_checktunnelsanity");
	}

	                                

	return 1;
}

                                                       
void
key_sa_recordxfer(
	struct secasvar *sav,
	struct mbuf *m)
{
	if (!sav) {
		panic("key_sa_recordxfer called with sav == NULL");
	}
	if (!m) {
		panic("key_sa_recordxfer called with m == NULL");
	}
	if (!sav->lft_c) {
		return;
	}

	lck_mtx_lock(sadb_mutex);
	  
                                                      
                                            
    
	sav->lft_c->sadb_lifetime_bytes += m->m_pkthdr.len;
	                                                           

	  
                                               
                                                         
                                            
    
	sav->lft_c->sadb_lifetime_allocations++;
	                            

	  
                                                                      
                                                                
                                                             
   
           
                         
                               
                         
                
    
	{
		struct timeval tv;
		microtime(&tv);
		sav->lft_c->sadb_lifetime_usetime = tv.tv_sec;
		                            
	}
	lck_mtx_unlock(sadb_mutex);

	return;
}

                  
void
key_sa_routechange(
	struct sockaddr *dst)
{
	struct secashead *sah;
	struct route *ro;

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sah, &sahtree, chain) {
		ro = (struct route *)&sah->sa_route;
		if (ro->ro_rt && dst->sa_len == ro->ro_dst.sa_len
		    && bcmp(dst, &ro->ro_dst, dst->sa_len) == 0) {
			ROUTE_RELEASE(ro);
		}
	}
	lck_mtx_unlock(sadb_mutex);

	return;
}

void
key_sa_chgstate(
	struct secasvar *sav,
	u_int8_t state)
{
	if (sav == NULL) {
		panic("key_sa_chgstate called with sav == NULL");
	}

	if (sav->state == state) {
		return;
	}

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_OWNED);

	if (__LIST_CHAINED(sav)) {
		LIST_REMOVE(sav, chain);
	}

	sav->state = state;
	LIST_INSERT_HEAD(&sav->sah->savtree[state], sav, chain);
}

void
key_sa_stir_iv(
	struct secasvar *sav)
{
	lck_mtx_lock(sadb_mutex);
	if (!sav->iv) {
		panic("key_sa_stir_iv called with sav == NULL");
	}
	key_randomfill(sav->iv, sav->ivlen);
	lck_mtx_unlock(sadb_mutex);
}

                   
static struct mbuf *
key_alloc_mbuf(
	int l)
{
	struct mbuf *m = NULL, *n;
	int len, t;

	len = l;
	while (len > 0) {
		MGET(n, M_DONTWAIT, MT_DATA);
		if (n && len > MLEN) {
			MCLGET(n, M_DONTWAIT);
		}
		if (!n) {
			m_freem(m);
			return NULL;
		}

		n->m_next = NULL;
		n->m_len = 0;
		n->m_len = (int)M_TRAILINGSPACE(n);
		                                                              
		if (n->m_len > len) {
			t = (n->m_len - len) & ~(sizeof(long) - 1);
			n->m_data += t;
			n->m_len = len;
		}

		len -= n->m_len;

		if (m) {
			m_cat(m, n);
		} else {
			m = n;
		}
	}

	return m;
}

static struct mbuf *
key_setdumpsastats(u_int32_t      dir,
    struct sastat *stats,
    u_int32_t      max_stats,
    u_int64_t      session_ids[],
    u_int32_t      seq,
    u_int32_t      pid)
{
	struct mbuf *result = NULL, *m = NULL;

	m = key_setsadbmsg(SADB_GETSASTAT, 0, 0, seq, pid, 0);
	if (!m) {
		goto fail;
	}
	result = m;

	m = key_setsadbsession_id(session_ids);
	if (!m) {
		goto fail;
	}
	m_cat(result, m);

	m = key_setsadbsastat(dir,
	    stats,
	    max_stats);
	if (!m) {
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	if (PFKEY_UNIT64(result->m_pkthdr.len) > UINT16_MAX) {
		ipseclog((LOG_ERR, "key_setdumpsastats: length too nbug: %u", result->m_pkthdr.len));
		goto fail;
	}

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return result;

fail:
	if (result) {
		m_freem(result);
	}
	return NULL;
}

  
                            
                                              
  
                          
   

static int
key_getsastat(struct socket *so,
    struct mbuf *m,
    const struct sadb_msghdr *mhp)
{
	struct sadb_session_id *session_id;
	size_t                  bufsize = 0;
	u_int32_t               arg_count, res_count;
	struct sadb_sastat     *sa_stats_arg;
	struct sastat          *sa_stats_sav = NULL;
	struct mbuf            *n;
	int                     error = 0;

	                  
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL) {
		panic("%s: NULL pointer is passed.\n", __FUNCTION__);
	}

	if (mhp->ext[SADB_EXT_SESSION_ID] == NULL) {
		printf("%s: invalid message is passed. missing session-id.\n", __FUNCTION__);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SESSION_ID] < sizeof(struct sadb_session_id)) {
		printf("%s: invalid message is passed. short session-id.\n", __FUNCTION__);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_EXT_SASTAT] == NULL) {
		printf("%s: invalid message is passed. missing stat args.\n", __FUNCTION__);
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SASTAT] < sizeof(*sa_stats_arg)) {
		printf("%s: invalid message is passed. short stat args.\n", __FUNCTION__);
		return key_senderror(so, m, EINVAL);
	}

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                                        
	if (ipsec_sav_count == 0) {
		printf("%s: No active SAs.\n", __FUNCTION__);
		error = ENOENT;
		goto end;
	}

	if (os_mul_overflow(ipsec_sav_count + 1, sizeof(*sa_stats_sav), &bufsize)) {
		panic("key_getsastat bufsize requested memory overflow %u\n", ipsec_sav_count);
	}

	KMALLOC_WAIT(sa_stats_sav, __typeof__(sa_stats_sav), bufsize);
	if (sa_stats_sav == NULL) {
		printf("%s: No more memory.\n", __FUNCTION__);
		error = ENOMEM;
		goto end;
	}
	bzero(sa_stats_sav, bufsize);

	sa_stats_arg = (__typeof__(sa_stats_arg))
	    (void *)mhp->ext[SADB_EXT_SASTAT];
	arg_count = sa_stats_arg->sadb_sastat_list_len;
	                                           
	if (arg_count == 0) {
		printf("%s: No SAs requested.\n", __FUNCTION__);
		error = ENOENT;
		goto end;
	}
	if (PFKEY_UNUNIT64(sa_stats_arg->sadb_sastat_len) < (sizeof(*sa_stats_arg) +
	    (arg_count * sizeof(struct sastat)))) {
		printf("%s: invalid message is passed. sa stat extlen shorter than requested stat length.\n", __FUNCTION__);
		error = EINVAL;
		goto end;
	}

	res_count = 0;

	if (key_getsastatbyspi((struct sastat *)(sa_stats_arg + 1),
	    arg_count,
	    sa_stats_sav,
	    bufsize,
	    &res_count)) {
		printf("%s: Error finding SAs.\n", __FUNCTION__);
		error = ENOENT;
		goto end;
	}
	if (!res_count) {
		printf("%s: No SAs found.\n", __FUNCTION__);
		error = ENOENT;
		goto end;
	}

	session_id = (__typeof__(session_id))
	    (void *)mhp->ext[SADB_EXT_SESSION_ID];

	                                
	n = key_setdumpsastats(sa_stats_arg->sadb_sastat_dir,
	    sa_stats_sav,
	    res_count,
	    session_id->sadb_session_id_v,
	    mhp->msg->sadb_msg_seq,
	    mhp->msg->sadb_msg_pid);
	if (!n) {
		printf("%s: No bufs to dump stats.\n", __FUNCTION__);
		error = ENOBUFS;
		goto end;
	}

	key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
end:
	if (sa_stats_sav) {
		KFREE(sa_stats_sav);
	}

	if (error) {
		return key_senderror(so, m, error);
	}

	m_freem(m);
	return 0;
}

static void
key_update_natt_keepalive_timestamp(struct secasvar *sav_sent,
    struct secasvar *sav_update)
{
	struct secasindex saidx_swap_sent_addr;

	                                                                   
	if (sav_sent == sav_update ||
	    sav_update->natt_last_activity == natt_now) {
		return;
	}

	                                                                                         

	bzero(&saidx_swap_sent_addr, sizeof(saidx_swap_sent_addr));
	memcpy(&saidx_swap_sent_addr.src, &sav_sent->sah->saidx.dst, sizeof(saidx_swap_sent_addr.src));
	memcpy(&saidx_swap_sent_addr.dst, &sav_sent->sah->saidx.src, sizeof(saidx_swap_sent_addr.dst));
	saidx_swap_sent_addr.proto = sav_sent->sah->saidx.proto;
	saidx_swap_sent_addr.mode = sav_sent->sah->saidx.mode;
	                                          

	if (key_cmpsaidx(&sav_sent->sah->saidx, &sav_update->sah->saidx, CMP_MODE | CMP_PORT) ||
	    key_cmpsaidx(&saidx_swap_sent_addr, &sav_update->sah->saidx, CMP_MODE | CMP_PORT)) {
		sav_update->natt_last_activity = natt_now;
	}
}

static int
key_send_delsp(struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;

	if (sp == NULL) {
		goto fail;
	}

	                    
	m = key_setsadbmsg(SADB_X_SPDDELETE, 0, 0, 0, 0, 0);
	if (!m) {
		goto fail;
	}
	result = m;

	                                     
	if (sp->spidx.src_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_START,
		    (struct sockaddr *)&sp->spidx.src_range.start, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_SRC_END,
		    (struct sockaddr *)&sp->spidx.src_range.end, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
		    (struct sockaddr *)&sp->spidx.src, sp->spidx.prefs,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	                                          
	if (sp->spidx.dst_range.start.ss_len > 0) {
		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_START,
		    (struct sockaddr *)&sp->spidx.dst_range.start, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);

		m = key_setsadbaddr(SADB_X_EXT_ADDR_RANGE_DST_END,
		    (struct sockaddr *)&sp->spidx.dst_range.end, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	} else {
		m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
		    (struct sockaddr *)&sp->spidx.dst, sp->spidx.prefd,
		    sp->spidx.ul_proto);
		if (!m) {
			goto fail;
		}
		m_cat(result, m);
	}

	                   
	m = key_sp2msg(sp);
	if (!m) {
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next) {
		result->m_pkthdr.len += m->m_len;
	}

	if (PFKEY_UNIT64(result->m_pkthdr.len) >= UINT16_MAX) {
		ipseclog((LOG_ERR, "key_send_delsp: length too big: %d", result->m_pkthdr.len));
		goto fail;
	}

	mtod(result, struct sadb_msg *)->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

fail:
	if (result) {
		m_free(result);
	}
	return -1;
}

void
key_delsp_for_ipsec_if(ifnet_t ipsec_if)
{
	struct secashead *sah;
	struct secasvar *sav, *nextsav;
	u_int stateidx;
	u_int state;
	struct secpolicy *sp, *nextsp;
	int dir;

	if (ipsec_if == NULL) {
		return;
	}

	LCK_MTX_ASSERT(sadb_mutex, LCK_MTX_ASSERT_NOTOWNED);

	lck_mtx_lock(sadb_mutex);

	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		for (sp = LIST_FIRST(&sptree[dir]);
		    sp != NULL;
		    sp = nextsp) {
			nextsp = LIST_NEXT(sp, chain);

			if (sp->ipsec_if == ipsec_if) {
				ifnet_release(sp->ipsec_if);
				sp->ipsec_if = NULL;

				key_send_delsp(sp);

				sp->state = IPSEC_SPSTATE_DEAD;
				key_freesp(sp, KEY_SADB_LOCKED);
			}
		}
	}

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->ipsec_if == ipsec_if) {
			                                                                       
			ifnet_release(sah->ipsec_if);
			sah->ipsec_if = NULL;

			for (stateidx = 0; stateidx < _ARRAYLEN(saorder_state_alive); stateidx++) {
				state = saorder_state_any[stateidx];
				for (sav = LIST_FIRST(&sah->savtree[state]); sav != NULL; sav = nextsav) {
					nextsav = LIST_NEXT(sav, chain);

					key_sa_chgstate(sav, SADB_SASTATE_DEAD);
					key_freesav(sav, KEY_SADB_LOCKED);
				}
			}

			sah->state = SADB_SASTATE_DEAD;
		}
	}

	lck_mtx_unlock(sadb_mutex);
}

__private_extern__ u_int32_t
key_fill_offload_frames_for_savs(ifnet_t ifp,
    struct ifnet_keepalive_offload_frame *frames_array,
    u_int32_t frames_array_count,
    size_t frame_data_offset)
{
	struct secashead *sah = NULL;
	struct secasvar *sav = NULL;
	struct ifnet_keepalive_offload_frame *frame = frames_array;
	u_int32_t frame_index = 0;

	if (frame == NULL || frames_array_count == 0) {
		return frame_index;
	}

	lck_mtx_lock(sadb_mutex);
	LIST_FOREACH(sah, &sahtree, chain) {
		LIST_FOREACH(sav, &sah->savtree[SADB_SASTATE_MATURE], chain) {
			if (ipsec_fill_offload_frame(ifp, sav, frame, frame_data_offset)) {
				frame_index++;
				if (frame_index >= frames_array_count) {
					lck_mtx_unlock(sadb_mutex);
					return frame_index;
				}
				frame = &(frames_array[frame_index]);
			}
		}
	}
	lck_mtx_unlock(sadb_mutex);

	return frame_index;
}

#pragma mark Custom IPsec

__private_extern__ bool
key_custom_ipsec_token_is_valid(void *ipsec_token)
{
	if (ipsec_token == NULL) {
		return false;
	}

	struct secashead *sah = (struct secashead *)ipsec_token;

	return (sah->flags & SECURITY_ASSOCIATION_CUSTOM_IPSEC) == SECURITY_ASSOCIATION_CUSTOM_IPSEC;
}

__private_extern__ int
key_reserve_custom_ipsec(void **ipsec_token, union sockaddr_in_4_6 *src, union sockaddr_in_4_6 *dst,
    u_int8_t proto)
{
	if (src == NULL || dst == NULL) {
		ipseclog((LOG_ERR, "register custom ipsec: invalid address\n"));
		return EINVAL;
	}

	if (src->sa.sa_family != dst->sa.sa_family) {
		ipseclog((LOG_ERR, "register custom ipsec: address family mismatched\n"));
		return EINVAL;
	}

	if (src->sa.sa_len != dst->sa.sa_len) {
		ipseclog((LOG_ERR, "register custom ipsec: address struct size mismatched\n"));
		return EINVAL;
	}

	if (ipsec_token == NULL) {
		ipseclog((LOG_ERR, "register custom ipsec: invalid ipsec token\n"));
		return EINVAL;
	}

	switch (src->sa.sa_family) {
	case AF_INET:
		if (src->sa.sa_len != sizeof(struct sockaddr_in)) {
			ipseclog((LOG_ERR, "register custom esp: invalid address length\n"));
			return EINVAL;
		}
		break;
	case AF_INET6:
		if (src->sa.sa_len != sizeof(struct sockaddr_in6)) {
			ipseclog((LOG_ERR, "register custom esp: invalid address length\n"));
			return EINVAL;
		}
		break;
	default:
		ipseclog((LOG_ERR, "register custom esp: invalid address length\n"));
		return EAFNOSUPPORT;
	}

	if (proto != IPPROTO_ESP && proto != IPPROTO_AH) {
		ipseclog((LOG_ERR, "register custom esp: invalid proto %u\n", proto));
		return EINVAL;
	}

	struct secasindex saidx = {};
	KEY_SETSECASIDX(proto, IPSEC_MODE_ANY, 0, &src->sa, &dst->sa, 0, &saidx);

	lck_mtx_lock(sadb_mutex);

	struct secashead *sah = NULL;
	if ((sah = key_getsah(&saidx, SECURITY_ASSOCIATION_ANY)) != NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_ERR, "register custom esp: SA exists\n"));
		return EEXIST;
	}

	if ((sah = key_newsah(&saidx, NULL, 0, IPSEC_DIR_ANY, SECURITY_ASSOCIATION_CUSTOM_IPSEC)) == NULL) {
		lck_mtx_unlock(sadb_mutex);
		ipseclog((LOG_DEBUG, "register custom esp: No more memory.\n"));
		return ENOBUFS;
	}

	*ipsec_token = (void *)sah;

	lck_mtx_unlock(sadb_mutex);
	return 0;
}

__private_extern__ void
key_release_custom_ipsec(void **ipsec_token)
{
	struct secashead *sah = *ipsec_token;
	VERIFY(sah != NULL);

	lck_mtx_lock(sadb_mutex);

	VERIFY((sah->flags & SECURITY_ASSOCIATION_CUSTOM_IPSEC) == SECURITY_ASSOCIATION_CUSTOM_IPSEC);

	bool sa_present = true;
	if (LIST_FIRST(&sah->savtree[SADB_SASTATE_LARVAL]) == NULL &&
	    LIST_FIRST(&sah->savtree[SADB_SASTATE_MATURE]) == NULL &&
	    LIST_FIRST(&sah->savtree[SADB_SASTATE_DYING]) == NULL &&
	    LIST_FIRST(&sah->savtree[SADB_SASTATE_DEAD]) == NULL) {
		sa_present = false;
	}
	VERIFY(sa_present == false);

	key_delsah(sah);

	lck_mtx_unlock(sadb_mutex);

	*ipsec_token = NULL;
	return;
}
