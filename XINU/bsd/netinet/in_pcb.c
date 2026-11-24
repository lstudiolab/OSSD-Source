/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
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
 *	@(#)in_pcb.c	8.4 (Berkeley) 5/24/95
 * $FreeBSD: src/sys/netinet/in_pcb.c,v 1.59.2.17 2001/08/13 16:26:17 ume Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/mcache.h>
#include <sys/kauth.h>
#include <sys/priv.h>
#include <sys/proc_uuid_policy.h>
#include <sys/syslog.h>
#include <sys/priv.h>
#include <net/dlil.h>

#include <libkern/OSAtomic.h>
#include <kern/locks.h>

#include <machine/limits.h>

#include <kern/zalloc.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/flowhash.h>
#include <net/flowadv.h>
#include <net/nat464_utils.h>
#include <net/ntstat.h>
#include <net/restricted_in_port.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <sys/kdebug.h>
#include <sys/random.h>

#include <dev/random/randomdev.h>
#include <mach/boolean.h>

#include <pexpert/pexpert.h>

#if NECP
#include <net/necp.h>
#endif

#include <sys/stat.h>
#include <sys/ubc.h>
#include <sys/vnode.h>

#include <os/log.h>

extern const char *proc_name_address(struct proc *);

static lck_grp_t        *inpcb_lock_grp;
static lck_attr_t       *inpcb_lock_attr;
static lck_grp_attr_t   *inpcb_lock_grp_attr;
decl_lck_mtx_data(static, inpcb_lock);                                 
decl_lck_mtx_data(static, inpcb_timeout_lock);

static TAILQ_HEAD(, inpcbinfo) inpcb_head = TAILQ_HEAD_INITIALIZER(inpcb_head);

static u_int16_t inpcb_timeout_run = 0;                                      
static boolean_t inpcb_garbage_collecting = FALSE;                            
static boolean_t inpcb_ticking = FALSE;                                        
static boolean_t inpcb_fast_timer_on = FALSE;

#define INPCB_GCREQ_THRESHOLD   50000

static thread_call_t inpcb_thread_call, inpcb_fast_thread_call;
static void inpcb_sched_timeout(void);
static void inpcb_sched_lazy_timeout(void);
static void _inpcb_sched_timeout(unsigned int);
static void inpcb_timeout(void *, void *);
const int inpcb_timeout_lazy = 10;                                             
extern int tvtohz(struct timeval *);

#if CONFIG_PROC_UUID_POLICY
static void inp_update_cellular_policy(struct inpcb *, boolean_t);
#if NECP
static void inp_update_necp_want_app_policy(struct inpcb *, boolean_t);
#endif           
#endif                               

#define DBG_FNC_PCB_LOOKUP      NETDBG_CODE(DBG_NETTCP, (6 << 8))
#define DBG_FNC_PCB_HLOOKUP     NETDBG_CODE(DBG_NETTCP, ((6 << 8) | 1))

int allow_udp_port_exhaustion = 0;

  
                                                                
                                                       
   
int     ipport_lowfirstauto  = IPPORT_RESERVED - 1;               
int     ipport_lowlastauto = IPPORT_RESERVEDSTART;               
int     ipport_firstauto = IPPORT_HIFIRSTAUTO;                     
int     ipport_lastauto  = IPPORT_HILASTAUTO;                      
int     ipport_hifirstauto = IPPORT_HIFIRSTAUTO;                   
int     ipport_hilastauto  = IPPORT_HILASTAUTO;                    

#define RANGECHK(var, min, max) \
	if ((var) < (min)) { (var) = (min); } \
	else if ((var) > (max)) { (var) = (max); }

static int
sysctl_net_ipport_check SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error;
	int new_value = *(int *)oidp->oid_arg1;
#if (DEBUG | DEVELOPMENT)
	int old_value = *(int *)oidp->oid_arg1;
	  
                                                           
                                              
    
	if (req->newptr) {
		if (proc_suser(current_proc()) != 0 &&
		    (error = priv_check_cred(kauth_cred_get(),
		    PRIV_NETINET_RESERVEDPORT, 0))) {
			return EPERM;
		}
	}
#endif                            

	error = sysctl_handle_int(oidp, &new_value, 0, req);
	if (!error) {
		if (oidp->oid_arg1 == &ipport_lowfirstauto || oidp->oid_arg1 == &ipport_lowlastauto) {
			RANGECHK(new_value, 1, IPPORT_RESERVED - 1);
		} else {
			RANGECHK(new_value, IPPORT_RESERVED, USHRT_MAX);
		}
		*(int *)oidp->oid_arg1 = new_value;
	}

#if (DEBUG | DEVELOPMENT)
	os_log(OS_LOG_DEFAULT,
	    "%s:%u sysctl net.restricted_port.verbose: %d -> %d)",
	    proc_best_name(current_proc()), proc_selfpid(),
	    old_value, *(int *)oidp->oid_arg1);
#endif                            

	return error;
}

#undef RANGECHK

SYSCTL_NODE(_net_inet_ip, IPPROTO_IP, portrange,
    CTLFLAG_RW | CTLFLAG_LOCKED, 0, "IP Ports");

#if (DEBUG | DEVELOPMENT)
#define CTLFAGS_IP_PORTRANGE (CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_ANYBODY)
#else
#define CTLFAGS_IP_PORTRANGE (CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED)
#endif                            

SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, lowfirst,
    CTLFAGS_IP_PORTRANGE,
    &ipport_lowfirstauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, lowlast,
    CTLFAGS_IP_PORTRANGE,
    &ipport_lowlastauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, first,
    CTLFAGS_IP_PORTRANGE,
    &ipport_firstauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, last,
    CTLFAGS_IP_PORTRANGE,
    &ipport_lastauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, hifirst,
    CTLFAGS_IP_PORTRANGE,
    &ipport_hifirstauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_PROC(_net_inet_ip_portrange, OID_AUTO, hilast,
    CTLFAGS_IP_PORTRANGE,
    &ipport_hilastauto, 0, &sysctl_net_ipport_check, "I", "");
SYSCTL_INT(_net_inet_ip_portrange, OID_AUTO, ipport_allow_udp_port_exhaustion,
    CTLFLAG_LOCKED | CTLFLAG_RW, &allow_udp_port_exhaustion, 0, "");

static uint32_t apn_fallbk_debug = 0;
#define apn_fallbk_log(x)       do { if (apn_fallbk_debug >= 1) log x; } while (0)

#if !XNU_TARGET_OS_OSX
static boolean_t apn_fallbk_enabled = TRUE;

SYSCTL_DECL(_net_inet);
SYSCTL_NODE(_net_inet, OID_AUTO, apn_fallback, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "APN Fallback");
SYSCTL_UINT(_net_inet_apn_fallback, OID_AUTO, enable, CTLFLAG_RW | CTLFLAG_LOCKED,
    &apn_fallbk_enabled, 0, "APN fallback enable");
SYSCTL_UINT(_net_inet_apn_fallback, OID_AUTO, debug, CTLFLAG_RW | CTLFLAG_LOCKED,
    &apn_fallbk_debug, 0, "APN fallback debug enable");
#else                        
static boolean_t apn_fallbk_enabled = FALSE;
#endif                        

extern int      udp_use_randomport;
extern int      tcp_use_randomport;

                                           
struct inp_flowhash_key_addr {
	union {
		struct in_addr  v4;
		struct in6_addr v6;
		u_int8_t        addr8[16];
		u_int16_t       addr16[8];
		u_int32_t       addr32[4];
	} infha;
};

struct inp_flowhash_key {
	struct inp_flowhash_key_addr    infh_laddr;
	struct inp_flowhash_key_addr    infh_faddr;
	u_int32_t                       infh_lport;
	u_int32_t                       infh_fport;
	u_int32_t                       infh_af;
	u_int32_t                       infh_proto;
	u_int32_t                       infh_rand1;
	u_int32_t                       infh_rand2;
};

static u_int32_t inp_hash_seed = 0;

static int infc_cmp(const struct inpcb *, const struct inpcb *);

                                 
#define INPFC_SOLOCKED  0x1
#define INPFC_REMOVE    0x2
static struct inpcb *inp_fc_getinp(u_int32_t, u_int32_t);

static void inp_fc_feedback(struct inpcb *);
extern void tcp_remove_from_time_wait(struct inpcb *inp);

decl_lck_mtx_data(static, inp_fc_lck);

RB_HEAD(inp_fc_tree, inpcb) inp_fc_tree;
RB_PROTOTYPE(inp_fc_tree, inpcb, infc_link, infc_cmp);
RB_GENERATE(inp_fc_tree, inpcb, infc_link, infc_cmp);

  
                                                             
                                              
   
struct inpcb key_inp;

  
                                                
   

void
in_pcbinit(void)
{
	static int inpcb_initialized = 0;

	VERIFY(!inpcb_initialized);
	inpcb_initialized = 1;

	inpcb_lock_grp_attr = lck_grp_attr_alloc_init();
	inpcb_lock_grp = lck_grp_alloc_init("inpcb", inpcb_lock_grp_attr);
	inpcb_lock_attr = lck_attr_alloc_init();
	lck_mtx_init(&inpcb_lock, inpcb_lock_grp, inpcb_lock_attr);
	lck_mtx_init(&inpcb_timeout_lock, inpcb_lock_grp, inpcb_lock_attr);
	inpcb_thread_call = thread_call_allocate_with_priority(inpcb_timeout,
	    NULL, THREAD_CALL_PRIORITY_KERNEL);
	                                                                
	inpcb_fast_thread_call = thread_call_allocate_with_priority(
		inpcb_timeout, &inpcb_timeout, THREAD_CALL_PRIORITY_KERNEL);
	if (inpcb_thread_call == NULL || inpcb_fast_thread_call == NULL) {
		panic("unable to alloc the inpcb thread call");
	}

	  
                                                  
                    
    
	lck_mtx_init(&inp_fc_lck, inpcb_lock_grp, inpcb_lock_attr);
	lck_mtx_lock(&inp_fc_lck);
	RB_INIT(&inp_fc_tree);
	bzero(&key_inp, sizeof(key_inp));
	lck_mtx_unlock(&inp_fc_lck);
}

#define INPCB_HAVE_TIMER_REQ(req)       (((req).intimer_lazy > 0) || \
	((req).intimer_fast > 0) || ((req).intimer_nodelay > 0))
static void
inpcb_timeout(void *arg0, void *arg1)
{
#pragma unused(arg1)
	struct inpcbinfo *ipi;
	boolean_t t, gc;
	struct intimercount gccnt, tmcnt;

	  
                                                                  
                                                                 
                                
    
	net_update_uptime();

	bzero(&gccnt, sizeof(gccnt));
	bzero(&tmcnt, sizeof(tmcnt));

	lck_mtx_lock_spin(&inpcb_timeout_lock);
	gc = inpcb_garbage_collecting;
	inpcb_garbage_collecting = FALSE;

	t = inpcb_ticking;
	inpcb_ticking = FALSE;

	if (gc || t) {
		lck_mtx_unlock(&inpcb_timeout_lock);

		lck_mtx_lock(&inpcb_lock);
		TAILQ_FOREACH(ipi, &inpcb_head, ipi_entry) {
			if (INPCB_HAVE_TIMER_REQ(ipi->ipi_gc_req)) {
				bzero(&ipi->ipi_gc_req,
				    sizeof(ipi->ipi_gc_req));
				if (gc && ipi->ipi_gc != NULL) {
					ipi->ipi_gc(ipi);
					gccnt.intimer_lazy +=
					    ipi->ipi_gc_req.intimer_lazy;
					gccnt.intimer_fast +=
					    ipi->ipi_gc_req.intimer_fast;
					gccnt.intimer_nodelay +=
					    ipi->ipi_gc_req.intimer_nodelay;
				}
			}
			if (INPCB_HAVE_TIMER_REQ(ipi->ipi_timer_req)) {
				bzero(&ipi->ipi_timer_req,
				    sizeof(ipi->ipi_timer_req));
				if (t && ipi->ipi_timer != NULL) {
					ipi->ipi_timer(ipi);
					tmcnt.intimer_lazy +=
					    ipi->ipi_timer_req.intimer_lazy;
					tmcnt.intimer_fast +=
					    ipi->ipi_timer_req.intimer_fast;
					tmcnt.intimer_nodelay +=
					    ipi->ipi_timer_req.intimer_nodelay;
				}
			}
		}
		lck_mtx_unlock(&inpcb_lock);
		lck_mtx_lock_spin(&inpcb_timeout_lock);
	}

	                                                              
	if (!inpcb_garbage_collecting) {
		inpcb_garbage_collecting = INPCB_HAVE_TIMER_REQ(gccnt);
	}
	if (!inpcb_ticking) {
		inpcb_ticking = INPCB_HAVE_TIMER_REQ(tmcnt);
	}

	                                               
	if (arg0 != NULL) {
		inpcb_fast_timer_on = FALSE;
	}
	inpcb_timeout_run--;
	VERIFY(inpcb_timeout_run >= 0 && inpcb_timeout_run < 2);

	                                            
	if (gccnt.intimer_nodelay > 0 || tmcnt.intimer_nodelay > 0) {
		inpcb_sched_timeout();
	} else if ((gccnt.intimer_fast + tmcnt.intimer_fast) <= 5) {
		                                            
		inpcb_sched_lazy_timeout();
	} else {
		inpcb_sched_timeout();
	}

	lck_mtx_unlock(&inpcb_timeout_lock);
}

static void
inpcb_sched_timeout(void)
{
	_inpcb_sched_timeout(0);
}

static void
inpcb_sched_lazy_timeout(void)
{
	_inpcb_sched_timeout(inpcb_timeout_lazy);
}

static void
_inpcb_sched_timeout(unsigned int offset)
{
	uint64_t deadline, leeway;

	clock_interval_to_deadline(1, NSEC_PER_SEC, &deadline);
	LCK_MTX_ASSERT(&inpcb_timeout_lock, LCK_MTX_ASSERT_OWNED);
	if (inpcb_timeout_run == 0 &&
	    (inpcb_garbage_collecting || inpcb_ticking)) {
		lck_mtx_convert_spin(&inpcb_timeout_lock);
		inpcb_timeout_run++;
		if (offset == 0) {
			inpcb_fast_timer_on = TRUE;
			thread_call_enter_delayed(inpcb_fast_thread_call,
			    deadline);
		} else {
			inpcb_fast_timer_on = FALSE;
			clock_interval_to_absolutetime_interval(offset,
			    NSEC_PER_SEC, &leeway);
			thread_call_enter_delayed_with_leeway(
				inpcb_thread_call, NULL, deadline, leeway,
				THREAD_CALL_DELAY_LEEWAY);
		}
	} else if (inpcb_timeout_run == 1 &&
	    offset == 0 && !inpcb_fast_timer_on) {
		  
                                                   
                                                     
                                         
     
		lck_mtx_convert_spin(&inpcb_timeout_lock);
		inpcb_timeout_run++;
		inpcb_fast_timer_on = TRUE;
		thread_call_enter_delayed(inpcb_fast_thread_call, deadline);
	}
}

void
inpcb_gc_sched(struct inpcbinfo *ipi, u_int32_t type)
{
	u_int32_t gccnt;

	lck_mtx_lock_spin(&inpcb_timeout_lock);
	inpcb_garbage_collecting = TRUE;
	gccnt = ipi->ipi_gc_req.intimer_nodelay +
	    ipi->ipi_gc_req.intimer_fast;

	if (gccnt > INPCB_GCREQ_THRESHOLD) {
		type = INPCB_TIMER_FAST;
	}

	switch (type) {
	case INPCB_TIMER_NODELAY:
		atomic_add_32(&ipi->ipi_gc_req.intimer_nodelay, 1);
		inpcb_sched_timeout();
		break;
	case INPCB_TIMER_FAST:
		atomic_add_32(&ipi->ipi_gc_req.intimer_fast, 1);
		inpcb_sched_timeout();
		break;
	default:
		atomic_add_32(&ipi->ipi_gc_req.intimer_lazy, 1);
		inpcb_sched_lazy_timeout();
		break;
	}
	lck_mtx_unlock(&inpcb_timeout_lock);
}

void
inpcb_timer_sched(struct inpcbinfo *ipi, u_int32_t type)
{
	lck_mtx_lock_spin(&inpcb_timeout_lock);
	inpcb_ticking = TRUE;
	switch (type) {
	case INPCB_TIMER_NODELAY:
		atomic_add_32(&ipi->ipi_timer_req.intimer_nodelay, 1);
		inpcb_sched_timeout();
		break;
	case INPCB_TIMER_FAST:
		atomic_add_32(&ipi->ipi_timer_req.intimer_fast, 1);
		inpcb_sched_timeout();
		break;
	default:
		atomic_add_32(&ipi->ipi_timer_req.intimer_lazy, 1);
		inpcb_sched_lazy_timeout();
		break;
	}
	lck_mtx_unlock(&inpcb_timeout_lock);
}

void
in_pcbinfo_attach(struct inpcbinfo *ipi)
{
	struct inpcbinfo *ipi0;

	lck_mtx_lock(&inpcb_lock);
	TAILQ_FOREACH(ipi0, &inpcb_head, ipi_entry) {
		if (ipi0 == ipi) {
			panic("%s: ipi %p already in the list\n",
			    __func__, ipi);
			                
		}
	}
	TAILQ_INSERT_TAIL(&inpcb_head, ipi, ipi_entry);
	lck_mtx_unlock(&inpcb_lock);
}

int
in_pcbinfo_detach(struct inpcbinfo *ipi)
{
	struct inpcbinfo *ipi0;
	int error = 0;

	lck_mtx_lock(&inpcb_lock);
	TAILQ_FOREACH(ipi0, &inpcb_head, ipi_entry) {
		if (ipi0 == ipi) {
			break;
		}
	}
	if (ipi0 != NULL) {
		TAILQ_REMOVE(&inpcb_head, ipi0, ipi_entry);
	} else {
		error = ENXIO;
	}
	lck_mtx_unlock(&inpcb_lock);

	return error;
}

  
                                                   
  
                       
           
          
   
int
in_pcballoc(struct socket *so, struct inpcbinfo *pcbinfo, struct proc *p)
{
#pragma unused(p)
	struct inpcb *inp;
	caddr_t temp;

	if ((so->so_flags1 & SOF1_CACHED_IN_SOCK_LAYER) == 0) {
		inp = (struct inpcb *)zalloc(pcbinfo->ipi_zone);
		if (inp == NULL) {
			return ENOBUFS;
		}
		bzero((caddr_t)inp, sizeof(*inp));
	} else {
		inp = (struct inpcb *)(void *)so->so_saved_pcb;
		temp = inp->inp_saved_ppcb;
		bzero((caddr_t)inp, sizeof(*inp));
		inp->inp_saved_ppcb = temp;
	}

	inp->inp_gencnt = ++pcbinfo->ipi_gencnt;
	inp->inp_pcbinfo = pcbinfo;
	inp->inp_socket = so;
	                                                 
	inp->inp_stat = (struct inp_stat *)P2ROUNDUP(inp->inp_stat_store,
	    sizeof(u_int64_t));
	if (((uintptr_t)inp->inp_stat - (uintptr_t)inp->inp_stat_store) +
	    sizeof(*inp->inp_stat) > sizeof(inp->inp_stat_store)) {
		panic("%s: insufficient space to align inp_stat", __func__);
		                
	}

	                                                  
	inp->inp_cstat = (struct inp_stat *)P2ROUNDUP(inp->inp_cstat_store,
	    sizeof(u_int64_t));
	if (((uintptr_t)inp->inp_cstat - (uintptr_t)inp->inp_cstat_store) +
	    sizeof(*inp->inp_cstat) > sizeof(inp->inp_cstat_store)) {
		panic("%s: insufficient space to align inp_cstat", __func__);
		                
	}

	                                                  
	inp->inp_wstat = (struct inp_stat *)P2ROUNDUP(inp->inp_wstat_store,
	    sizeof(u_int64_t));
	if (((uintptr_t)inp->inp_wstat - (uintptr_t)inp->inp_wstat_store) +
	    sizeof(*inp->inp_wstat) > sizeof(inp->inp_wstat_store)) {
		panic("%s: insufficient space to align inp_wstat", __func__);
		                
	}

	                                                  
	inp->inp_Wstat = (struct inp_stat *)P2ROUNDUP(inp->inp_Wstat_store,
	    sizeof(u_int64_t));
	if (((uintptr_t)inp->inp_Wstat - (uintptr_t)inp->inp_Wstat_store) +
	    sizeof(*inp->inp_Wstat) > sizeof(inp->inp_Wstat_store)) {
		panic("%s: insufficient space to align inp_Wstat", __func__);
		                
	}

	so->so_pcb = (caddr_t)inp;

	if (so->so_proto->pr_flags & PR_PCBLOCK) {
		lck_mtx_init(&inp->inpcb_mtx, pcbinfo->ipi_lock_grp,
		    pcbinfo->ipi_lock_attr);
	}

	if (SOCK_DOM(so) == PF_INET6 && !ip6_mapped_addr_on) {
		inp->inp_flags |= IN6P_IPV6_V6ONLY;
	}

	if (ip6_auto_flowlabel) {
		inp->inp_flags |= IN6P_AUTOFLOWLABEL;
	}
	if (intcoproc_unrestricted) {
		inp->inp_flags2 |= INP2_INTCOPROC_ALLOWED;
	}

	(void) inp_update_policy(inp);

	lck_rw_lock_exclusive(pcbinfo->ipi_lock);
	inp->inp_gencnt = ++pcbinfo->ipi_gencnt;
	LIST_INSERT_HEAD(pcbinfo->ipi_listhead, inp, inp_list);
	pcbinfo->ipi_count++;
	lck_rw_done(pcbinfo->ipi_lock);
	return 0;
}

  
                                                 
                                                     
                                                    
                                                   
                                                  
                                                  
                  
   
struct inpcb *
in_pcblookup_local_and_cleanup(struct inpcbinfo *pcbinfo, struct in_addr laddr,
    u_int lport_arg, int wild_okay)
{
	struct inpcb *inp;

	                           
	inp = in_pcblookup_local(pcbinfo, laddr, lport_arg, wild_okay);

	                                                               
	if (inp != NULL && inp->inp_wantcnt == WNT_STOPUSING) {
		struct socket *so = inp->inp_socket;

		socket_lock(so, 0);

		if (so->so_usecount == 0) {
			if (inp->inp_state != INPCB_STATE_DEAD) {
				in_pcbdetach(inp);
			}
			in_pcbdispose(inp);                                
			inp = NULL;
		} else {
			socket_unlock(so, 0);
		}
	}

	return inp;
}

static void
in_pcb_conflict_post_msg(u_int16_t port)
{
	  
                                                       
                                                            
                                        
    
	struct kev_msg ev_msg;
	struct kev_in_portinuse in_portinuse;

	bzero(&in_portinuse, sizeof(struct kev_in_portinuse));
	bzero(&ev_msg, sizeof(struct kev_msg));
	in_portinuse.port = ntohs(port);                                
	in_portinuse.req_pid = proc_selfpid();
	ev_msg.vendor_code = KEV_VENDOR_APPLE;
	ev_msg.kev_class = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass = KEV_INET_SUBCLASS;
	ev_msg.event_code = KEV_INET_PORTINUSE;
	ev_msg.dv[0].data_ptr = &in_portinuse;
	ev_msg.dv[0].data_length = sizeof(struct kev_in_portinuse);
	ev_msg.dv[1].data_length = 0;
	dlil_post_complete_msg(NULL, &ev_msg);
}

  
                                                                          
                                           
  
                       
                                         
                             
                                                       
                              
                              
                                            
                                                 
   
int
in_pcbbind(struct inpcb *inp, struct sockaddr *nam, struct proc *p)
{
	struct socket *so = inp->inp_socket;
	unsigned short *lastport;
	struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
	u_short lport = 0, rand_port = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
	int error, randomport, conflict = 0;
	boolean_t anonport = FALSE;
	kauth_cred_t cred;
	struct in_addr laddr;
	struct ifnet *outif = NULL;

	if (TAILQ_EMPTY(&in_ifaddrhead)) {                  
		return EADDRNOTAVAIL;
	}
	if (!(so->so_options & (SO_REUSEADDR | SO_REUSEPORT))) {
		wild = 1;
	}

	bzero(&laddr, sizeof(laddr));

	socket_unlock(so, 0);                               
	lck_rw_lock_exclusive(pcbinfo->ipi_lock);
	if (inp->inp_lport != 0 || inp->inp_laddr.s_addr != INADDR_ANY) {
		                                       
		lck_rw_done(pcbinfo->ipi_lock);
		socket_lock(so, 0);
		return EINVAL;
	}

	if (nam != NULL) {
		if (nam->sa_len != sizeof(struct sockaddr_in)) {
			lck_rw_done(pcbinfo->ipi_lock);
			socket_lock(so, 0);
			return EINVAL;
		}
#if 0
		  
                                                 
                                       
     
		if (nam->sa_family != AF_INET) {
			lck_rw_done(pcbinfo->ipi_lock);
			socket_lock(so, 0);
			return EAFNOSUPPORT;
		}
#endif        
		lport = SIN(nam)->sin_port;

		if (IN_MULTICAST(ntohl(SIN(nam)->sin_addr.s_addr))) {
			  
                                                       
                                              
                                                    
                                              
                                 
      
			if (so->so_options & SO_REUSEADDR) {
				reuseport = SO_REUSEADDR | SO_REUSEPORT;
			}
		} else if (SIN(nam)->sin_addr.s_addr != INADDR_ANY) {
			struct sockaddr_in sin;
			struct ifaddr *ifa;

			                                              
			bzero(&sin, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_len = sizeof(struct sockaddr_in);
			sin.sin_addr.s_addr = SIN(nam)->sin_addr.s_addr;

			ifa = ifa_ifwithaddr(SA(&sin));
			if (ifa == NULL) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return EADDRNOTAVAIL;
			} else {
				  
                                               
                                               
                                           
                                             
                                             
                                            
       
				IFA_LOCK(ifa);
				outif = ifa->ifa_ifp;
				IFA_UNLOCK(ifa);
				IFA_REMREF(ifa);
			}
		}


		if (lport != 0) {
			struct inpcb *t;
			uid_t u;

#if XNU_TARGET_OS_OSX
			if (ntohs(lport) < IPPORT_RESERVED &&
			    SIN(nam)->sin_addr.s_addr != 0 &&
			    !(inp->inp_flags2 & INP2_EXTERNAL_PORT)) {
				cred = kauth_cred_proc_ref(p);
				error = priv_check_cred(cred,
				    PRIV_NETINET_RESERVEDPORT, 0);
				kauth_cred_unref(&cred);
				if (error != 0) {
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EACCES;
				}
			}
#endif                        
			  
                                                                      
      
			if (!current_task_can_use_restricted_in_port(lport,
			    (uint8_t)so->so_proto->pr_protocol, PORT_FLAGS_BSD)) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return EADDRINUSE;
			}

			if (!IN_MULTICAST(ntohl(SIN(nam)->sin_addr.s_addr)) &&
			    (u = kauth_cred_getuid(so->so_cred)) != 0 &&
			    (t = in_pcblookup_local_and_cleanup(
				    inp->inp_pcbinfo, SIN(nam)->sin_addr, lport,
				    INPLOOKUP_WILDCARD)) != NULL &&
			    (SIN(nam)->sin_addr.s_addr != INADDR_ANY ||
			    t->inp_laddr.s_addr != INADDR_ANY ||
			    !(t->inp_socket->so_options & SO_REUSEPORT)) &&
			    (u != kauth_cred_getuid(t->inp_socket->so_cred)) &&
			    !(t->inp_socket->so_flags & SOF_REUSESHAREUID) &&
			    (SIN(nam)->sin_addr.s_addr != INADDR_ANY ||
			    t->inp_laddr.s_addr != INADDR_ANY) &&
			    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
				if ((t->inp_socket->so_flags &
				    SOF_NOTIFYCONFLICT) &&
				    !(so->so_flags & SOF_NOTIFYCONFLICT)) {
					conflict = 1;
				}

				lck_rw_done(pcbinfo->ipi_lock);

				if (conflict) {
					in_pcb_conflict_post_msg(lport);
				}

				socket_lock(so, 0);
				return EADDRINUSE;
			}
			t = in_pcblookup_local_and_cleanup(pcbinfo,
			    SIN(nam)->sin_addr, lport, wild);
			if (t != NULL &&
			    (reuseport & t->inp_socket->so_options) == 0 &&
			    (!(t->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    !(inp->inp_flags2 & INP2_EXTERNAL_PORT) ||
			    uuid_compare(t->necp_client_uuid, inp->necp_client_uuid) != 0)) {
				if (SIN(nam)->sin_addr.s_addr != INADDR_ANY ||
				    t->inp_laddr.s_addr != INADDR_ANY ||
				    SOCK_DOM(so) != PF_INET6 ||
				    SOCK_DOM(t->inp_socket) != PF_INET6) {
					if ((t->inp_socket->so_flags &
					    SOF_NOTIFYCONFLICT) &&
					    !(so->so_flags & SOF_NOTIFYCONFLICT)) {
						conflict = 1;
					}

					lck_rw_done(pcbinfo->ipi_lock);

					if (conflict) {
						in_pcb_conflict_post_msg(lport);
					}
					socket_lock(so, 0);
					return EADDRINUSE;
				}
			}
		}
		laddr = SIN(nam)->sin_addr;
	}
	if (lport == 0) {
		u_short first, last;
		int count;
		bool found;

		  
                                                                 
                                                                  
                                       
     
		wild = 1;

		randomport = (so->so_flags & SOF_BINDRANDOMPORT) ||
		    (so->so_type == SOCK_STREAM ? tcp_use_randomport :
		    udp_use_randomport);

		  
                                                  
                                                       
     
		anonport = TRUE;
		if (inp->inp_flags & INP_HIGHPORT) {
			first = (u_short)ipport_hifirstauto;                 
			last  = (u_short)ipport_hilastauto;
			lastport = &pcbinfo->ipi_lasthi;
		} else if (inp->inp_flags & INP_LOWPORT) {
			cred = kauth_cred_proc_ref(p);
			error = priv_check_cred(cred,
			    PRIV_NETINET_RESERVEDPORT, 0);
			kauth_cred_unref(&cred);
			if (error != 0) {
				lck_rw_done(pcbinfo->ipi_lock);
				socket_lock(so, 0);
				return error;
			}
			first = (u_short)ipport_lowfirstauto;              
			last  = (u_short)ipport_lowlastauto;              
			lastport = &pcbinfo->ipi_lastlow;
		} else {
			first = (u_short)ipport_firstauto;                   
			last  = (u_short)ipport_lastauto;
			lastport = &pcbinfo->ipi_lastport;
		}
		                                                           

		if (first == last) {
			randomport = 0;
		}
		  
                                                             
                     
    
                                                               
                                                   
     
		if (first > last) {
			struct in_addr lookup_addr;

			  
                   
      
			if (randomport) {
				read_frandom(&rand_port, sizeof(rand_port));
				*lastport =
				    first - (rand_port % (first - last));
			}
			count = first - last;

			lookup_addr = (laddr.s_addr != INADDR_ANY) ? laddr :
			    inp->inp_laddr;

			found = false;
			do {
				if (count-- < 0) {                            
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EADDRNOTAVAIL;
				}
				--*lastport;
				if (*lastport > first || *lastport < last) {
					*lastport = first;
				}
				lport = htons(*lastport);

				  
                                                             
                                    
       
				if (IS_RESTRICTED_IN_PORT(lport)) {
					continue;
				}

				found = in_pcblookup_local_and_cleanup(pcbinfo,
				    lookup_addr, lport, wild) == NULL;
			} while (!found);
		} else {
			struct in_addr lookup_addr;

			  
                 
      
			if (randomport) {
				read_frandom(&rand_port, sizeof(rand_port));
				*lastport =
				    first + (rand_port % (first - last));
			}
			count = last - first;

			lookup_addr = (laddr.s_addr != INADDR_ANY) ? laddr :
			    inp->inp_laddr;

			found = false;
			do {
				if (count-- < 0) {                            
					lck_rw_done(pcbinfo->ipi_lock);
					socket_lock(so, 0);
					return EADDRNOTAVAIL;
				}
				++*lastport;
				if (*lastport < first || *lastport > last) {
					*lastport = first;
				}
				lport = htons(*lastport);

				  
                                                             
                                    
       
				if (IS_RESTRICTED_IN_PORT(lport)) {
					continue;
				}

				found = in_pcblookup_local_and_cleanup(pcbinfo,
				    lookup_addr, lport, wild) == NULL;
			} while (!found);
		}
	}
	socket_lock(so, 0);

	  
                                                       
                                                 
                                        
    
	if (inp->inp_state == INPCB_STATE_DEAD) {
		lck_rw_done(pcbinfo->ipi_lock);
		return ECONNABORTED;
	}

	if (inp->inp_lport != 0 || inp->inp_laddr.s_addr != INADDR_ANY) {
		lck_rw_done(pcbinfo->ipi_lock);
		return EINVAL;
	}

	if (laddr.s_addr != INADDR_ANY) {
		inp->inp_laddr = laddr;
		inp->inp_last_outifp = outif;
	}
	inp->inp_lport = lport;
	if (anonport) {
		inp->inp_flags |= INP_ANONPORT;
	}

	if (in_pcbinshash(inp, 1) != 0) {
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_last_outifp = NULL;

		inp->inp_lport = 0;
		if (anonport) {
			inp->inp_flags &= ~INP_ANONPORT;
		}
		lck_rw_done(pcbinfo->ipi_lock);
		return EAGAIN;
	}
	lck_rw_done(pcbinfo->ipi_lock);
	sflt_notify(so, sock_evt_bound, NULL);
	return 0;
}

#define APN_FALLBACK_IP_FILTER(a)       \
	(IN_LINKLOCAL(ntohl((a)->sin_addr.s_addr)) || \
	 IN_LOOPBACK(ntohl((a)->sin_addr.s_addr)) || \
	 IN_ZERONET(ntohl((a)->sin_addr.s_addr)) || \
	 IN_MULTICAST(ntohl((a)->sin_addr.s_addr)) || \
	 IN_PRIVATE(ntohl((a)->sin_addr.s_addr)))

#define APN_FALLBACK_NOTIF_INTERVAL     2                   
static uint64_t last_apn_fallback = 0;

static boolean_t
apn_fallback_required(proc_t proc, struct socket *so, struct sockaddr_in *p_dstv4)
{
	uint64_t timenow;
	struct sockaddr_storage lookup_default_addr;
	struct rtentry *rt = NULL;

	VERIFY(proc != NULL);

	if (apn_fallbk_enabled == FALSE) {
		return FALSE;
	}

	if (proc == kernproc) {
		return FALSE;
	}

	if (so && (so->so_options & SO_NOAPNFALLBK)) {
		return FALSE;
	}

	timenow = net_uptime();
	if ((timenow - last_apn_fallback) < APN_FALLBACK_NOTIF_INTERVAL) {
		apn_fallbk_log((LOG_INFO, "APN fallback notification throttled.\n"));
		return FALSE;
	}

	if (p_dstv4 && APN_FALLBACK_IP_FILTER(p_dstv4)) {
		return FALSE;
	}

	                                                                   
	bzero(&lookup_default_addr, sizeof(lookup_default_addr));
	lookup_default_addr.ss_family = AF_INET6;
	lookup_default_addr.ss_len = sizeof(struct sockaddr_in6);

	rt = rtalloc1((struct sockaddr *)&lookup_default_addr, 0, 0);
	if (NULL == rt) {
		apn_fallbk_log((LOG_INFO, "APN fallback notification could not find "
		    "unscoped default IPv6 route.\n"));
		return FALSE;
	}

	if (!IFNET_IS_CELLULAR(rt->rt_ifp)) {
		rtfree(rt);
		apn_fallbk_log((LOG_INFO, "APN fallback notification could not find "
		    "unscoped default IPv6 route through cellular interface.\n"));
		return FALSE;
	}

	  
                                             
                                                       
             
    
	rtfree(rt);
	rt = NULL;

	bzero(&lookup_default_addr, sizeof(lookup_default_addr));
	lookup_default_addr.ss_family = AF_INET;
	lookup_default_addr.ss_len = sizeof(struct sockaddr_in);

	rt = rtalloc1((struct sockaddr *)&lookup_default_addr, 0, 0);

	if (rt) {
		rtfree(rt);
		rt = NULL;
		apn_fallbk_log((LOG_INFO, "APN fallback notification found unscoped "
		    "IPv4 default route!\n"));
		return FALSE;
	}

	{
		  
                                                                    
                                                           
                                                        
     
		const char *bundle_id = cs_identity_get(proc);
		if (bundle_id == NULL ||
		    bundle_id[0] == '\0' ||
		    strchr(bundle_id, '.') == NULL ||
		    strncmp(bundle_id, "com.apple.", sizeof("com.apple.") - 1) == 0) {
			apn_fallbk_log((LOG_INFO, "Abort: APN fallback notification found first-"
			    "party bundle ID \"%s\"!\n", (bundle_id ? bundle_id : "NULL")));
			return FALSE;
		}
	}

	{
		  
                                                    
                                       
                                                                    
                                                                               
     
		static const long ipv6_start_date = 1464764400L;
		vfs_context_t context;
		struct stat64 sb;
		int vn_stat_error;

		bzero(&sb, sizeof(struct stat64));
		context = vfs_context_create(NULL);
		vn_stat_error = vn_stat(proc->p_textvp, &sb, NULL, 1, 0, context);
		(void)vfs_context_rele(context);

		if (vn_stat_error != 0 ||
		    sb.st_atimespec.tv_sec >= ipv6_start_date ||
		    sb.st_birthtimespec.tv_sec >= ipv6_start_date) {
			apn_fallbk_log((LOG_INFO, "Abort: APN fallback notification found binary "
			    "too recent! (err %d atime %ld mtime %ld ctime %ld birthtime %ld)\n",
			    vn_stat_error, sb.st_atimespec.tv_sec, sb.st_mtimespec.tv_sec,
			    sb.st_ctimespec.tv_sec, sb.st_birthtimespec.tv_sec));
			return FALSE;
		}
	}
	return TRUE;
}

static void
apn_fallback_trigger(proc_t proc, struct socket *so)
{
	pid_t pid = 0;
	struct kev_msg ev_msg;
	struct kev_netevent_apnfallbk_data apnfallbk_data;

	last_apn_fallback = net_uptime();
	pid = proc_pid(proc);
	uuid_t application_uuid;
	uuid_clear(application_uuid);
	proc_getexecutableuuid(proc, application_uuid,
	    sizeof(application_uuid));

	bzero(&ev_msg, sizeof(struct kev_msg));
	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     = KEV_NETEVENT_SUBCLASS;
	ev_msg.event_code       = KEV_NETEVENT_APNFALLBACK;

	bzero(&apnfallbk_data, sizeof(apnfallbk_data));

	if (so->so_flags & SOF_DELEGATED) {
		apnfallbk_data.epid = so->e_pid;
		uuid_copy(apnfallbk_data.euuid, so->e_uuid);
	} else {
		apnfallbk_data.epid = so->last_pid;
		uuid_copy(apnfallbk_data.euuid, so->last_uuid);
	}

	ev_msg.dv[0].data_ptr   = &apnfallbk_data;
	ev_msg.dv[0].data_length = sizeof(apnfallbk_data);
	kev_post_msg(&ev_msg);
	apn_fallbk_log((LOG_INFO, "APN fallback notification issued.\n"));
}

  
                                                                 
                                                                   
                                                                
                                                
  
                                                                   
  
                                                                       
                                                              
  
                                                                       
                                                                    
                                           
  
                       
                             
                                              
                                        
   
int
in_pcbladdr(struct inpcb *inp, struct sockaddr *nam, struct in_addr *laddr,
    unsigned int ifscope, struct ifnet **outif, int raw)
{
	struct route *ro = &inp->inp_route;
	struct in_ifaddr *ia = NULL;
	struct sockaddr_in sin;
	int error = 0;
	boolean_t restricted = FALSE;

	if (outif != NULL) {
		*outif = NULL;
	}
	if (nam->sa_len != sizeof(struct sockaddr_in)) {
		return EINVAL;
	}
	if (SIN(nam)->sin_family != AF_INET) {
		return EAFNOSUPPORT;
	}
	if (raw == 0 && SIN(nam)->sin_port == 0) {
		return EADDRNOTAVAIL;
	}

	  
                                             
                                  
                                                
                                                 
                                                    
    
	if (raw == 0 && (SIN(nam)->sin_addr.s_addr == INADDR_ANY ||
	    SIN(nam)->sin_addr.s_addr == (u_int32_t)INADDR_BROADCAST)) {
		lck_rw_lock_shared(in_ifaddr_rwlock);
		if (!TAILQ_EMPTY(&in_ifaddrhead)) {
			ia = TAILQ_FIRST(&in_ifaddrhead);
			IFA_LOCK_SPIN(&ia->ia_ifa);
			if (SIN(nam)->sin_addr.s_addr == INADDR_ANY) {
				SIN(nam)->sin_addr = IA_SIN(ia)->sin_addr;
			} else if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
				SIN(nam)->sin_addr =
				    SIN(&ia->ia_broadaddr)->sin_addr;
			}
			IFA_UNLOCK(&ia->ia_ifa);
			ia = NULL;
		}
		lck_rw_done(in_ifaddr_rwlock);
	}
	  
                                                                       
    
	if (inp->inp_laddr.s_addr != INADDR_ANY) {
		VERIFY(ia == NULL);
		*laddr = inp->inp_laddr;
		return 0;
	}

	  
                                                               
                                                            
    
	if (ifscope == IFSCOPE_NONE && (inp->inp_flags & INP_BOUND_IF)) {
		ifscope = inp->inp_boundifp->if_index;
	}

	  
                                              
                                                  
                                                              
                                                        
    
	if (ro->ro_rt != NULL) {
		RT_LOCK_SPIN(ro->ro_rt);
	}
	if (ROUTE_UNUSABLE(ro) || ro->ro_dst.sa_family != AF_INET ||
	    SIN(&ro->ro_dst)->sin_addr.s_addr != SIN(nam)->sin_addr.s_addr ||
	    (inp->inp_socket->so_options & SO_DONTROUTE)) {
		if (ro->ro_rt != NULL) {
			RT_UNLOCK(ro->ro_rt);
		}
		ROUTE_RELEASE(ro);
	}
	if (!(inp->inp_socket->so_options & SO_DONTROUTE) &&
	    (ro->ro_rt == NULL || ro->ro_rt->rt_ifp == NULL)) {
		if (ro->ro_rt != NULL) {
			RT_UNLOCK(ro->ro_rt);
		}
		ROUTE_RELEASE(ro);
		                                         
		bzero(&ro->ro_dst, sizeof(struct sockaddr_in));
		ro->ro_dst.sa_family = AF_INET;
		ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
		SIN(&ro->ro_dst)->sin_addr = SIN(nam)->sin_addr;
		rtalloc_scoped(ro, ifscope);
		if (ro->ro_rt != NULL) {
			RT_LOCK_SPIN(ro->ro_rt);
		}
	}
	                                                         
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_addr.s_addr = SIN(nam)->sin_addr.s_addr;
	  
                                                                 
                                                                   
                                              
    
	if (ro->ro_rt == NULL) {
		proc_t proc = current_proc();

		VERIFY(ia == NULL);
		ia = ifatoia(ifa_ifwithdstaddr(SA(&sin)));
		if (ia == NULL) {
			ia = ifatoia(ifa_ifwithnet_scoped(SA(&sin), ifscope));
		}
		error = ((ia == NULL) ? ENETUNREACH : 0);

		if (apn_fallback_required(proc, inp->inp_socket,
		    (void *)nam)) {
			apn_fallback_trigger(proc, inp->inp_socket);
		}

		goto done;
	}
	RT_LOCK_ASSERT_HELD(ro->ro_rt);
	  
                                                       
                                                              
    
	if (!(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK)) {
		VERIFY(ia == NULL);
		  
                                                        
                                                      
                                    
                                                   
     
		if (inp_restricted_send(inp, ro->ro_rt->rt_ifp)) {
			RT_UNLOCK(ro->ro_rt);
			ROUTE_RELEASE(ro);
			error = EHOSTUNREACH;
			restricted = TRUE;
		} else {
			                            
			RT_CONVERT_LOCK(ro->ro_rt);
			ia = ifatoia(ro->ro_rt->rt_ifa);
			IFA_ADDREF(&ia->ia_ifa);

			  
                                                
                                               
                  
     
                                                 
                                  
                                                          
                                             
      
			if (IS_INTF_CLAT46(ro->ro_rt->rt_ifp)) {
				inp->inp_flags2 |= INP2_CLAT46_FLOW;
			}

			RT_UNLOCK(ro->ro_rt);
			error = 0;
		}
		goto done;
	}
	VERIFY(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK);
	RT_UNLOCK(ro->ro_rt);
	  
                                                                    
                         
                                                                 
                                                                  
                         
    
	VERIFY(ia == NULL);
	ia = ifatoia(ifa_ifwithdstaddr(SA(&sin)));
	if (ia == NULL) {
		ia = ifatoia(ifa_ifwithaddr_scoped(SA(&sin), ifscope));
	}
	if (ia == NULL) {
		ia = ifatoia(ifa_ifwithnet_scoped(SA(&sin), ifscope));
	}
	if (ia == NULL) {
		RT_LOCK(ro->ro_rt);
		ia = ifatoia(ro->ro_rt->rt_ifa);
		if (ia != NULL) {
			IFA_ADDREF(&ia->ia_ifa);
		}
		RT_UNLOCK(ro->ro_rt);
	}
	error = ((ia == NULL) ? ENETUNREACH : 0);

done:
	  
                                                           
                                                         
                                                    
    
	if (IN_MULTICAST(ntohl(SIN(nam)->sin_addr.s_addr)) &&
	    inp->inp_moptions != NULL) {
		struct ip_moptions *imo;
		struct ifnet *ifp;

		imo = inp->inp_moptions;
		IMO_LOCK(imo);
		if (imo->imo_multicast_ifp != NULL && (ia == NULL ||
		    ia->ia_ifp != imo->imo_multicast_ifp)) {
			ifp = imo->imo_multicast_ifp;
			if (ia != NULL) {
				IFA_REMREF(&ia->ia_ifa);
			}
			lck_rw_lock_shared(in_ifaddr_rwlock);
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_link) {
				if (ia->ia_ifp == ifp) {
					break;
				}
			}
			if (ia != NULL) {
				IFA_ADDREF(&ia->ia_ifa);
			}
			lck_rw_done(in_ifaddr_rwlock);
			if (ia == NULL) {
				error = EADDRNOTAVAIL;
			} else {
				error = 0;
			}
		}
		IMO_UNLOCK(imo);
	}
	  
                                                           
                                                
    
	if (ia != NULL) {
		  
                                                          
                                                        
                                                   
                                                   
     
		IFA_LOCK_SPIN(&ia->ia_ifa);
		if (inp_restricted_send(inp, ia->ia_ifa.ifa_ifp)) {
			IFA_UNLOCK(&ia->ia_ifa);
			error = EHOSTUNREACH;
			restricted = TRUE;
		} else if (error == 0) {
			*laddr = ia->ia_addr.sin_addr;
			if (outif != NULL) {
				struct ifnet *ifp;

				if (ro->ro_rt != NULL) {
					ifp = ro->ro_rt->rt_ifp;
				} else {
					ifp = ia->ia_ifp;
				}

				VERIFY(ifp != NULL);
				IFA_CONVERT_LOCK(&ia->ia_ifa);
				ifnet_reference(ifp);                   
				if (*outif != NULL) {
					ifnet_release(*outif);
				}
				*outif = ifp;
			}
			IFA_UNLOCK(&ia->ia_ifa);
		} else {
			IFA_UNLOCK(&ia->ia_ifa);
		}
		IFA_REMREF(&ia->ia_ifa);
		ia = NULL;
	}

	if (restricted && error == EHOSTUNREACH) {
		soevent(inp->inp_socket, (SO_FILT_HINT_LOCKED |
		    SO_FILT_HINT_IFDENIED));
	}

	return error;
}

  
                    
                                                
                                                           
                                                     
                 
  
                                                                       
                                                              
   
int
in_pcbconnect(struct inpcb *inp, struct sockaddr *nam, struct proc *p,
    unsigned int ifscope, struct ifnet **outif)
{
	struct in_addr laddr;
	struct sockaddr_in *sin = (struct sockaddr_in *)(void *)nam;
	struct inpcb *pcb;
	int error;
	struct socket *so = inp->inp_socket;

#if CONTENT_FILTER
	if (so) {
		so->so_state_change_cnt++;
	}
#endif

	  
                                                            
    
	if ((error = in_pcbladdr(inp, nam, &laddr, ifscope, outif, 0)) != 0) {
		return error;
	}

	socket_unlock(so, 0);
	pcb = in_pcblookup_hash(inp->inp_pcbinfo, sin->sin_addr, sin->sin_port,
	    inp->inp_laddr.s_addr ? inp->inp_laddr : laddr,
	    inp->inp_lport, 0, NULL);
	socket_lock(so, 0);

	  
                                                                      
                                                                     
                                 
    
	if ((so->so_flags & SOF_ABORTED) != 0) {
		return ECONNREFUSED;
	}

	if (pcb != NULL) {
		in_pcb_checkstate(pcb, WNT_RELEASE, pcb == inp ? 1 : 0);
		return EADDRINUSE;
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0) {
			error = in_pcbbind(inp, NULL, p);
			if (error) {
				return error;
			}
		}
		if (!lck_rw_try_lock_exclusive(inp->inp_pcbinfo->ipi_lock)) {
			  
                                           
                        
      
			socket_unlock(so, 0);
			lck_rw_lock_exclusive(inp->inp_pcbinfo->ipi_lock);
			socket_lock(so, 0);
		}
		inp->inp_laddr = laddr;
		                         
		inp->inp_last_outifp = (outif != NULL) ? *outif : NULL;
		inp->inp_flags |= INP_INADDR_ANY;
	} else {
		  
                                                    
                                            
                                 
                                                    
                                                           
               
     
		if (inp->inp_lport == 0) {
			return EINVAL;
		}
		if (!lck_rw_try_lock_exclusive(inp->inp_pcbinfo->ipi_lock)) {
			  
                                           
                        
      
			socket_unlock(so, 0);
			lck_rw_lock_exclusive(inp->inp_pcbinfo->ipi_lock);
			socket_lock(so, 0);
		}
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	if (nstat_collect && SOCK_PROTO(so) == IPPROTO_UDP) {
		nstat_pcb_invalidate_cache(inp);
	}
	in_pcbrehash(inp);
	lck_rw_done(inp->inp_pcbinfo->ipi_lock);
	return 0;
}

void
in_pcbdisconnect(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

	if (nstat_collect && SOCK_PROTO(so) == IPPROTO_UDP) {
		nstat_pcb_cache(inp);
	}

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;

#if CONTENT_FILTER
	if (so) {
		so->so_state_change_cnt++;
	}
#endif

	if (!lck_rw_try_lock_exclusive(inp->inp_pcbinfo->ipi_lock)) {
		                                                             
		socket_unlock(so, 0);
		lck_rw_lock_exclusive(inp->inp_pcbinfo->ipi_lock);
		socket_lock(so, 0);
	}

	in_pcbrehash(inp);
	lck_rw_done(inp->inp_pcbinfo->ipi_lock);
	  
                                                                        
                                                                     
                                                                        
    
	if (!(so->so_flags & SOF_MP_SUBFLOW) && (so->so_state & SS_NOFDREF)) {
		in_pcbdetach(inp);
	}
}

void
in_pcbdetach(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

	if (so->so_pcb == NULL) {
		                           
		panic("%s: inp=%p so=%p proto=%d so_pcb is null!\n", __func__,
		    inp, so, SOCK_PROTO(so));
		                
	}

#if IPSEC
	if (inp->inp_sp != NULL) {
		(void) ipsec4_delete_pcbpolicy(inp);
	}
#endif            

	if (inp->inp_stat != NULL && SOCK_PROTO(so) == IPPROTO_UDP) {
		if (inp->inp_stat->rxpackets == 0 && inp->inp_stat->txpackets == 0) {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet_dgram_no_data);
		}
	}

	  
                                                     
                        
    
	if (nstat_collect &&
	    (SOCK_PROTO(so) == IPPROTO_TCP || SOCK_PROTO(so) == IPPROTO_UDP)) {
		nstat_pcb_detach(inp);
	}

	                                                        
	if (inp->inp_keepalive_data != NULL) {
		FREE(inp->inp_keepalive_data, M_TEMP);
		inp->inp_keepalive_data = NULL;
	}

	                               
	if (in_pcb_checkstate(inp, WNT_STOPUSING, 1) != WNT_STOPUSING) {
		panic("%s: so=%p proto=%d couldn't set to STOPUSING\n",
		    __func__, so, SOCK_PROTO(so));
		                
	}

	if (!(so->so_flags & SOF_PCBCLEARING)) {
		struct ip_moptions *imo;

		inp->inp_vflag = 0;
		if (inp->inp_options != NULL) {
			(void) m_free(inp->inp_options);
			inp->inp_options = NULL;
		}
		ROUTE_RELEASE(&inp->inp_route);
		imo = inp->inp_moptions;
		inp->inp_moptions = NULL;
		sofreelastref(so, 0);
		inp->inp_state = INPCB_STATE_DEAD;

		  
                                                       
                                               
     
		if (inp->inp_flags2 & INP2_CLAT46_FLOW) {
			  
                                                  
                     
                                                    
                                    
      
			if (inp->inp_stat != NULL &&
			    (inp->inp_stat->txbytes != 0 ||
			    inp->inp_stat->rxbytes != 0)) {
				if (so->so_flags & SOF_DELEGATED) {
					in6_clat46_event_enqueue_nwk_wq_entry(
						IN6_CLAT46_EVENT_V4_FLOW,
						so->e_pid,
						so->e_uuid);
				} else {
					in6_clat46_event_enqueue_nwk_wq_entry(
						IN6_CLAT46_EVENT_V4_FLOW,
						so->last_pid,
						so->last_uuid);
				}
			}
		}

		                                                     
		so->so_flags |= SOF_PCBCLEARING;

		inpcb_gc_sched(inp->inp_pcbinfo, INPCB_TIMER_FAST);

		  
                                                   
     
		if (imo != NULL) {
			socket_unlock(so, 0);
			IMO_REMREF(imo);
			socket_lock(so, 0);
		}
	}
}


void
in_pcbdispose(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;
	struct inpcbinfo *ipi = inp->inp_pcbinfo;

	if (so != NULL && so->so_usecount != 0) {
		panic("%s: so %p [%d,%d] usecount %d lockhistory %s\n",
		    __func__, so, SOCK_DOM(so), SOCK_TYPE(so), so->so_usecount,
		    solockhistory_nr(so));
		                
	} else if (inp->inp_wantcnt != WNT_STOPUSING) {
		if (so != NULL) {
			panic_plain("%s: inp %p invalid wantcnt %d, so %p "
			    "[%d,%d] usecount %d retaincnt %d state 0x%x "
			    "flags 0x%x lockhistory %s\n", __func__, inp,
			    inp->inp_wantcnt, so, SOCK_DOM(so), SOCK_TYPE(so),
			    so->so_usecount, so->so_retaincnt, so->so_state,
			    so->so_flags, solockhistory_nr(so));
			                
		} else {
			panic("%s: inp %p invalid wantcnt %d no socket\n",
			    __func__, inp, inp->inp_wantcnt);
			                
		}
	}

	LCK_RW_ASSERT(ipi->ipi_lock, LCK_RW_ASSERT_EXCLUSIVE);

	inp->inp_gencnt = ++ipi->ipi_gencnt;
	                                  
	in_pcbremlists(inp);

	if (so != NULL) {
		if (so->so_proto->pr_flags & PR_PCBLOCK) {
			sofreelastref(so, 0);
			if (so->so_rcv.sb_cc > 0 || so->so_snd.sb_cc > 0) {
				  
                                      
                                    
       
				sbrelease(&so->so_rcv);
				sbrelease(&so->so_snd);
			}
			if (so->so_head != NULL) {
				panic("%s: so=%p head still exist\n",
				    __func__, so);
				                
			}
			lck_mtx_unlock(&inp->inpcb_mtx);

#if NECP
			necp_inpcb_remove_cb(inp);
#endif           

			lck_mtx_destroy(&inp->inpcb_mtx, ipi->ipi_lock_grp);
		}
		                                                     
		so->so_flags |= SOF_PCBCLEARING;
		so->so_saved_pcb = (caddr_t)inp;
		so->so_pcb = NULL;
		inp->inp_socket = NULL;
#if NECP
		necp_inpcb_dispose(inp);
#endif           
		  
                                                          
                                                        
                                 
     
		ROUTE_RELEASE(&inp->inp_route);
		if ((so->so_flags1 & SOF1_CACHED_IN_SOCK_LAYER) == 0) {
			zfree(ipi->ipi_zone, inp);
		}
		sodealloc(so);
	}
}

  
                                                                      
                                                                       
                                                                           
                                           
   
int
in_getsockaddr(struct socket *so, struct sockaddr **nam)
{
	struct inpcb *inp;
	struct sockaddr_in *sin;

	  
                                          
    
	MALLOC(sin, struct sockaddr_in *, sizeof(*sin), M_SONAME, M_WAITOK);
	if (sin == NULL) {
		return ENOBUFS;
	}
	bzero(sin, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);

	if ((inp = sotoinpcb(so)) == NULL) {
		FREE(sin, M_SONAME);
		return EINVAL;
	}
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;

	*nam = (struct sockaddr *)sin;
	return 0;
}

int
in_getsockaddr_s(struct socket *so, struct sockaddr_in *ss)
{
	struct sockaddr_in *sin = ss;
	struct inpcb *inp;

	VERIFY(ss != NULL);
	bzero(ss, sizeof(*ss));

	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);

	if ((inp = sotoinpcb(so)) == NULL) {
		return EINVAL;
	}

	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
	return 0;
}

int
in_getpeeraddr(struct socket *so, struct sockaddr **nam)
{
	struct inpcb *inp;
	struct sockaddr_in *sin;

	  
                                          
    
	MALLOC(sin, struct sockaddr_in *, sizeof(*sin), M_SONAME, M_WAITOK);
	if (sin == NULL) {
		return ENOBUFS;
	}
	bzero((caddr_t)sin, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);

	if ((inp = sotoinpcb(so)) == NULL) {
		FREE(sin, M_SONAME);
		return EINVAL;
	}
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;

	*nam = (struct sockaddr *)sin;
	return 0;
}

void
in_pcbnotifyall(struct inpcbinfo *pcbinfo, struct in_addr faddr,
    int errno, void (*notify)(struct inpcb *, int))
{
	struct inpcb *inp;

	lck_rw_lock_shared(pcbinfo->ipi_lock);

	LIST_FOREACH(inp, pcbinfo->ipi_listhead, inp_list) {
		if (!(inp->inp_vflag & INP_IPV4)) {
			continue;
		}
		if (inp->inp_faddr.s_addr != faddr.s_addr ||
		    inp->inp_socket == NULL) {
			continue;
		}
		if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) == WNT_STOPUSING) {
			continue;
		}
		socket_lock(inp->inp_socket, 1);
		(*notify)(inp, errno);
		(void) in_pcb_checkstate(inp, WNT_RELEASE, 1);
		socket_unlock(inp->inp_socket, 1);
	}
	lck_rw_done(pcbinfo->ipi_lock);
}

  
                                                     
                                                      
                                                             
                                                        
   
void
in_losing(struct inpcb *inp)
{
	boolean_t release = FALSE;
	struct rtentry *rt;

	if ((rt = inp->inp_route.ro_rt) != NULL) {
		struct in_ifaddr *ia = NULL;

		RT_LOCK(rt);
		if (rt->rt_flags & RTF_DYNAMIC) {
			  
                                                   
                                                  
                                              
      
			rt->rt_flags |= RTF_CONDEMNED;
			RT_UNLOCK(rt);
			(void) rtrequest(RTM_DELETE, rt_key(rt),
			    rt->rt_gateway, rt_mask(rt), rt->rt_flags, NULL);
		} else {
			RT_UNLOCK(rt);
		}
		                                                          
		if (inp->inp_laddr.s_addr != INADDR_ANY &&
		    (ia = ifa_foraddr(inp->inp_laddr.s_addr)) != NULL) {
			  
                                                      
                                                         
      
			release = TRUE;
		}
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
	}
	if (rt == NULL || release) {
		ROUTE_RELEASE(&inp->inp_route);
	}
}

  
                                            
                                         
   
void
in_rtchange(struct inpcb *inp, int errno)
{
#pragma unused(errno)
	boolean_t release = FALSE;
	struct rtentry *rt;

	if ((rt = inp->inp_route.ro_rt) != NULL) {
		struct in_ifaddr *ia = NULL;

		                                            
		if (inp->inp_laddr.s_addr != INADDR_ANY &&
		    (ia = ifa_foraddr(inp->inp_laddr.s_addr)) != NULL) {
			  
                                                      
                                                         
      
			release = TRUE;
		}
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
	}
	if (rt == NULL || release) {
		ROUTE_RELEASE(&inp->inp_route);
	}
}

  
                                                    
   
struct inpcb *
in_pcblookup_local(struct inpcbinfo *pcbinfo, struct in_addr laddr,
    unsigned int lport_arg, int wild_okay)
{
	struct inpcb *inp;
	int matchwild = 3, wildcard;
	u_short lport = (u_short)lport_arg;

	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_START, 0, 0, 0, 0, 0);

	if (!wild_okay) {
		struct inpcbhead *head;
		  
                                                             
                                                          
     
		head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
		    pcbinfo->ipi_hashmask)];
		LIST_FOREACH(inp, head, inp_hash) {
			if (!(inp->inp_vflag & INP_IPV4)) {
				continue;
			}
			if (inp->inp_faddr.s_addr == INADDR_ANY &&
			    inp->inp_laddr.s_addr == laddr.s_addr &&
			    inp->inp_lport == lport) {
				  
             
       
				return inp;
			}
		}
		  
               
     
		KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_END, 0, 0, 0, 0, 0);
		return NULL;
	} else {
		struct inpcbporthead *porthash;
		struct inpcbport *phd;
		struct inpcb *match = NULL;
		  
                         
    
                                                             
                    
     
		porthash = &pcbinfo->ipi_porthashbase[INP_PCBPORTHASH(lport,
		    pcbinfo->ipi_porthashmask)];
		LIST_FOREACH(phd, porthash, phd_hash) {
			if (phd->phd_port == lport) {
				break;
			}
		}
		if (phd != NULL) {
			  
                                                       
          
      
			LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist) {
				wildcard = 0;
				if (!(inp->inp_vflag & INP_IPV4)) {
					continue;
				}
				if (inp->inp_faddr.s_addr != INADDR_ANY) {
					wildcard++;
				}
				if (inp->inp_laddr.s_addr != INADDR_ANY) {
					if (laddr.s_addr == INADDR_ANY) {
						wildcard++;
					} else if (inp->inp_laddr.s_addr !=
					    laddr.s_addr) {
						continue;
					}
				} else {
					if (laddr.s_addr != INADDR_ANY) {
						wildcard++;
					}
				}
				if (wildcard < matchwild) {
					match = inp;
					matchwild = wildcard;
					if (matchwild == 0) {
						break;
					}
				}
			}
		}
		KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_END, match,
		    0, 0, 0, 0);
		return match;
	}
}

  
                                    
   
int
in_pcblookup_hash_exists(struct inpcbinfo *pcbinfo, struct in_addr faddr,
    u_int fport_arg, struct in_addr laddr, u_int lport_arg, int wildcard,
    uid_t *uid, gid_t *gid, struct ifnet *ifp)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	u_short fport = (u_short)fport_arg, lport = (u_short)lport_arg;
	int found = 0;
	struct inpcb *local_wild = NULL;
	struct inpcb *local_wild_mapped = NULL;

	*uid = UID_MAX;
	*gid = GID_MAX;

	  
                                                                    
    

	lck_rw_lock_shared(pcbinfo->ipi_lock);

	  
                                  
    
	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(faddr.s_addr, lport, fport,
	    pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV4)) {
			continue;
		}
		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (inp->inp_faddr.s_addr == faddr.s_addr &&
		    inp->inp_laddr.s_addr == laddr.s_addr &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport) {
			if ((found = (inp->inp_socket != NULL))) {
				  
             
       
				*uid = kauth_cred_getuid(
					inp->inp_socket->so_cred);
				*gid = kauth_cred_getgid(
					inp->inp_socket->so_cred);
			}
			lck_rw_done(pcbinfo->ipi_lock);
			return found;
		}
	}

	if (!wildcard) {
		  
               
     
		lck_rw_done(pcbinfo->ipi_lock);
		return 0;
	}

	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
	    pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV4)) {
			continue;
		}
		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (inp->inp_faddr.s_addr == INADDR_ANY &&
		    inp->inp_lport == lport) {
			if (inp->inp_laddr.s_addr == laddr.s_addr) {
				if ((found = (inp->inp_socket != NULL))) {
					*uid = kauth_cred_getuid(
						inp->inp_socket->so_cred);
					*gid = kauth_cred_getgid(
						inp->inp_socket->so_cred);
				}
				lck_rw_done(pcbinfo->ipi_lock);
				return found;
			} else if (inp->inp_laddr.s_addr == INADDR_ANY) {
				if (inp->inp_socket &&
				    SOCK_CHECK_DOM(inp->inp_socket, PF_INET6)) {
					local_wild_mapped = inp;
				} else {
					local_wild = inp;
				}
			}
		}
	}
	if (local_wild == NULL) {
		if (local_wild_mapped != NULL) {
			if ((found = (local_wild_mapped->inp_socket != NULL))) {
				*uid = kauth_cred_getuid(
					local_wild_mapped->inp_socket->so_cred);
				*gid = kauth_cred_getgid(
					local_wild_mapped->inp_socket->so_cred);
			}
			lck_rw_done(pcbinfo->ipi_lock);
			return found;
		}
		lck_rw_done(pcbinfo->ipi_lock);
		return 0;
	}
	if ((found = (local_wild->inp_socket != NULL))) {
		*uid = kauth_cred_getuid(
			local_wild->inp_socket->so_cred);
		*gid = kauth_cred_getgid(
			local_wild->inp_socket->so_cred);
	}
	lck_rw_done(pcbinfo->ipi_lock);
	return found;
}

  
                           
   
struct inpcb *
in_pcblookup_hash(struct inpcbinfo *pcbinfo, struct in_addr faddr,
    u_int fport_arg, struct in_addr laddr, u_int lport_arg, int wildcard,
    struct ifnet *ifp)
{
	struct inpcbhead *head;
	struct inpcb *inp;
	u_short fport = (u_short)fport_arg, lport = (u_short)lport_arg;
	struct inpcb *local_wild = NULL;
	struct inpcb *local_wild_mapped = NULL;

	  
                                                                    
    

	lck_rw_lock_shared(pcbinfo->ipi_lock);

	  
                                  
    
	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(faddr.s_addr, lport, fport,
	    pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV4)) {
			continue;
		}
		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (inp->inp_faddr.s_addr == faddr.s_addr &&
		    inp->inp_laddr.s_addr == laddr.s_addr &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport) {
			  
            
      
			if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) !=
			    WNT_STOPUSING) {
				lck_rw_done(pcbinfo->ipi_lock);
				return inp;
			} else {
				                                             
				lck_rw_done(pcbinfo->ipi_lock);
				return NULL;
			}
		}
	}

	if (!wildcard) {
		  
               
     
		lck_rw_done(pcbinfo->ipi_lock);
		return NULL;
	}

	head = &pcbinfo->ipi_hashbase[INP_PCBHASH(INADDR_ANY, lport, 0,
	    pcbinfo->ipi_hashmask)];
	LIST_FOREACH(inp, head, inp_hash) {
		if (!(inp->inp_vflag & INP_IPV4)) {
			continue;
		}
		if (inp_restricted_recv(inp, ifp)) {
			continue;
		}

#if NECP
		if (!necp_socket_is_allowed_to_recv_on_interface(inp, ifp)) {
			continue;
		}
#endif           

		if (inp->inp_faddr.s_addr == INADDR_ANY &&
		    inp->inp_lport == lport) {
			if (inp->inp_laddr.s_addr == laddr.s_addr) {
				if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) !=
				    WNT_STOPUSING) {
					lck_rw_done(pcbinfo->ipi_lock);
					return inp;
				} else {
					                                   
					lck_rw_done(pcbinfo->ipi_lock);
					return NULL;
				}
			} else if (inp->inp_laddr.s_addr == INADDR_ANY) {
				if (SOCK_CHECK_DOM(inp->inp_socket, PF_INET6)) {
					local_wild_mapped = inp;
				} else {
					local_wild = inp;
				}
			}
		}
	}
	if (local_wild == NULL) {
		if (local_wild_mapped != NULL) {
			if (in_pcb_checkstate(local_wild_mapped,
			    WNT_ACQUIRE, 0) != WNT_STOPUSING) {
				lck_rw_done(pcbinfo->ipi_lock);
				return local_wild_mapped;
			} else {
				                                   
				lck_rw_done(pcbinfo->ipi_lock);
				return NULL;
			}
		}
		lck_rw_done(pcbinfo->ipi_lock);
		return NULL;
	}
	if (in_pcb_checkstate(local_wild, WNT_ACQUIRE, 0) != WNT_STOPUSING) {
		lck_rw_done(pcbinfo->ipi_lock);
		return local_wild;
	}
	  
                                             
    
	lck_rw_done(pcbinfo->ipi_lock);
	return NULL;
}

  
                                             
  
                                                        
                                                          
                                         
  
                                                
   
int
in_pcbinshash(struct inpcb *inp, int locked)
{
	struct inpcbhead *pcbhash;
	struct inpcbporthead *pcbporthash;
	struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
	struct inpcbport *phd;
	u_int32_t hashkey_faddr;

	if (!locked) {
		if (!lck_rw_try_lock_exclusive(pcbinfo->ipi_lock)) {
			  
                                           
                       
      
			socket_unlock(inp->inp_socket, 0);
			lck_rw_lock_exclusive(pcbinfo->ipi_lock);
			socket_lock(inp->inp_socket, 0);
		}
	}

	  
                                                
                                   
                                                      
                               
    
	if (inp->inp_state == INPCB_STATE_DEAD) {
		if (!locked) {
			lck_rw_done(pcbinfo->ipi_lock);
		}
		return ECONNABORTED;
	}


	if (inp->inp_vflag & INP_IPV6) {
		hashkey_faddr = inp->in6p_faddr.s6_addr32[3]          ;
	} else {
		hashkey_faddr = inp->inp_faddr.s_addr;
	}

	inp->inp_hash_element = INP_PCBHASH(hashkey_faddr, inp->inp_lport,
	    inp->inp_fport, pcbinfo->ipi_hashmask);

	pcbhash = &pcbinfo->ipi_hashbase[inp->inp_hash_element];

	pcbporthash = &pcbinfo->ipi_porthashbase[INP_PCBPORTHASH(inp->inp_lport,
	    pcbinfo->ipi_porthashmask)];

	  
                                                            
    
	LIST_FOREACH(phd, pcbporthash, phd_hash) {
		if (phd->phd_port == inp->inp_lport) {
			break;
		}
	}

	  
                                              
    
	if (phd == NULL) {
		MALLOC(phd, struct inpcbport *, sizeof(struct inpcbport),
		    M_PCB, M_WAITOK);
		if (phd == NULL) {
			if (!locked) {
				lck_rw_done(pcbinfo->ipi_lock);
			}
			return ENOBUFS;          
		}
		phd->phd_port = inp->inp_lport;
		LIST_INIT(&phd->phd_pcblist);
		LIST_INSERT_HEAD(pcbporthash, phd, phd_hash);
	}

	VERIFY(!(inp->inp_flags2 & INP2_INHASHLIST));


	inp->inp_phd = phd;
	LIST_INSERT_HEAD(&phd->phd_pcblist, inp, inp_portlist);
	LIST_INSERT_HEAD(pcbhash, inp, inp_hash);
	inp->inp_flags2 |= INP2_INHASHLIST;

	if (!locked) {
		lck_rw_done(pcbinfo->ipi_lock);
	}

#if NECP
	                                                              
	inp_update_necp_policy(inp, NULL, NULL, 0);
#endif           

	return 0;
}

  
                                                                      
                                                                          
                                                                        
                                                    
   
void
in_pcbrehash(struct inpcb *inp)
{
	struct inpcbhead *head;
	u_int32_t hashkey_faddr;

	if (inp->inp_vflag & INP_IPV6) {
		hashkey_faddr = inp->in6p_faddr.s6_addr32[3]          ;
	} else {
		hashkey_faddr = inp->inp_faddr.s_addr;
	}

	inp->inp_hash_element = INP_PCBHASH(hashkey_faddr, inp->inp_lport,
	    inp->inp_fport, inp->inp_pcbinfo->ipi_hashmask);
	head = &inp->inp_pcbinfo->ipi_hashbase[inp->inp_hash_element];

	if (inp->inp_flags2 & INP2_INHASHLIST) {
		LIST_REMOVE(inp, inp_hash);
		inp->inp_flags2 &= ~INP2_INHASHLIST;
	}

	VERIFY(!(inp->inp_flags2 & INP2_INHASHLIST));
	LIST_INSERT_HEAD(head, inp, inp_hash);
	inp->inp_flags2 |= INP2_INHASHLIST;

#if NECP
	                                                    
	inp_update_necp_policy(inp, NULL, NULL, 0);
#endif           
}

  
                                 
                                                         
   
void
in_pcbremlists(struct inpcb *inp)
{
	inp->inp_gencnt = ++inp->inp_pcbinfo->ipi_gencnt;

	  
                                                                  
                                                               
                     
    
	if (inp->inp_flags2 & INP2_INHASHLIST) {
		struct inpcbport *phd = inp->inp_phd;

		VERIFY(phd != NULL && inp->inp_lport > 0);

		LIST_REMOVE(inp, inp_hash);
		inp->inp_hash.le_next = NULL;
		inp->inp_hash.le_prev = NULL;

		LIST_REMOVE(inp, inp_portlist);
		inp->inp_portlist.le_next = NULL;
		inp->inp_portlist.le_prev = NULL;
		if (LIST_EMPTY(&phd->phd_pcblist)) {
			LIST_REMOVE(phd, phd_hash);
			FREE(phd, M_PCB);
		}
		inp->inp_phd = NULL;
		inp->inp_flags2 &= ~INP2_INHASHLIST;
	}
	VERIFY(!(inp->inp_flags2 & INP2_INHASHLIST));

	if (inp->inp_flags2 & INP2_TIMEWAIT) {
		                                 
		tcp_remove_from_time_wait(inp);
		inp->inp_flags2 &= ~INP2_TIMEWAIT;
		VERIFY(inp->inp_pcbinfo->ipi_twcount != 0);
		inp->inp_pcbinfo->ipi_twcount--;
	} else {
		                                                        
		LIST_REMOVE(inp, inp_list);
	}

	if (inp->inp_flags2 & INP2_IN_FCTREE) {
		inp_fc_getinp(inp->inp_flowhash, (INPFC_SOLOCKED | INPFC_REMOVE));
		VERIFY(!(inp->inp_flags2 & INP2_IN_FCTREE));
	}

	inp->inp_pcbinfo->ipi_count--;
}

  
                                                     
                                                                        
                                    
                   
                          
                   
                                                         
   
int
in_pcb_checkstate(struct inpcb *pcb, int mode, int locked)
{
	volatile UInt32 *wantcnt = (volatile UInt32 *)&pcb->inp_wantcnt;
	UInt32 origwant;
	UInt32 newwant;

	switch (mode) {
	case WNT_STOPUSING:
		  
                                                          
                                                           
                    
     
		if (locked == 0) {
			socket_lock(pcb->inp_socket, 1);
		}
		pcb->inp_state = INPCB_STATE_DEAD;

stopusing:
		if (pcb->inp_socket->so_usecount < 0) {
			panic("%s: pcb=%p so=%p usecount is negative\n",
			    __func__, pcb, pcb->inp_socket);
			                
		}
		if (locked == 0) {
			socket_unlock(pcb->inp_socket, 1);
		}

		inpcb_gc_sched(pcb->inp_pcbinfo, INPCB_TIMER_FAST);

		origwant = *wantcnt;
		if ((UInt16) origwant == 0xffff) {                        
			return WNT_STOPUSING;
		}
		newwant = 0xffff;
		if ((UInt16) origwant == 0) {
			                                    
			OSCompareAndSwap(origwant, newwant, wantcnt);
		}
		return WNT_STOPUSING;

	case WNT_ACQUIRE:
		  
                                                        
                                                             
                                                        
     
		do {
			origwant = *wantcnt;
			if ((UInt16) origwant == 0xffff) {
				                       
				return WNT_STOPUSING;
			}
			newwant = origwant + 1;
		} while (!OSCompareAndSwap(origwant, newwant, wantcnt));
		return WNT_ACQUIRE;

	case WNT_RELEASE:
		  
                                                        
                                         
     
		if (locked == 0) {
			socket_lock(pcb->inp_socket, 1);
		}

		do {
			origwant = *wantcnt;
			if ((UInt16) origwant == 0x0) {
				panic("%s: pcb=%p release with zero count",
				    __func__, pcb);
				                
			}
			if ((UInt16) origwant == 0xffff) {
				                       
				if (locked == 0) {
					socket_unlock(pcb->inp_socket, 1);
				}
				return WNT_STOPUSING;
			}
			newwant = origwant - 1;
		} while (!OSCompareAndSwap(origwant, newwant, wantcnt));

		if (pcb->inp_state == INPCB_STATE_DEAD) {
			goto stopusing;
		}
		if (pcb->inp_socket->so_usecount < 0) {
			panic("%s: RELEASE pcb=%p so=%p usecount is negative\n",
			    __func__, pcb, pcb->inp_socket);
			                
		}

		if (locked == 0) {
			socket_unlock(pcb->inp_socket, 1);
		}
		return WNT_RELEASE;

	default:
		panic("%s: so=%p not a valid state =%x\n", __func__,
		    pcb->inp_socket, mode);
		                
	}

	                
	return mode;
}

  
                                                                      
                                                                   
                                                       
   
void
inpcb_to_compat(struct inpcb *inp, struct inpcb_compat *inp_compat)
{
	bzero(inp_compat, sizeof(*inp_compat));
	inp_compat->inp_fport = inp->inp_fport;
	inp_compat->inp_lport = inp->inp_lport;
	inp_compat->nat_owner = 0;
	inp_compat->nat_cookie = 0;
	inp_compat->inp_gencnt = inp->inp_gencnt;
	inp_compat->inp_flags = inp->inp_flags;
	inp_compat->inp_flow = inp->inp_flow;
	inp_compat->inp_vflag = inp->inp_vflag;
	inp_compat->inp_ip_ttl = inp->inp_ip_ttl;
	inp_compat->inp_ip_p = inp->inp_ip_p;
	inp_compat->inp_dependfaddr.inp6_foreign =
	    inp->inp_dependfaddr.inp6_foreign;
	inp_compat->inp_dependladdr.inp6_local =
	    inp->inp_dependladdr.inp6_local;
	inp_compat->inp_depend4.inp4_ip_tos = inp->inp_depend4.inp4_ip_tos;
	inp_compat->inp_depend6.inp6_hlim = 0;
	inp_compat->inp_depend6.inp6_cksum = inp->inp_depend6.inp6_cksum;
	inp_compat->inp_depend6.inp6_ifindex = 0;
	inp_compat->inp_depend6.inp6_hops = inp->inp_depend6.inp6_hops;
}

#if XNU_TARGET_OS_OSX
void
inpcb_to_xinpcb64(struct inpcb *inp, struct xinpcb64 *xinp)
{
	xinp->inp_fport = inp->inp_fport;
	xinp->inp_lport = inp->inp_lport;
	xinp->inp_gencnt = inp->inp_gencnt;
	xinp->inp_flags = inp->inp_flags;
	xinp->inp_flow = inp->inp_flow;
	xinp->inp_vflag = inp->inp_vflag;
	xinp->inp_ip_ttl = inp->inp_ip_ttl;
	xinp->inp_ip_p = inp->inp_ip_p;
	xinp->inp_dependfaddr.inp6_foreign = inp->inp_dependfaddr.inp6_foreign;
	xinp->inp_dependladdr.inp6_local = inp->inp_dependladdr.inp6_local;
	xinp->inp_depend4.inp4_ip_tos = inp->inp_depend4.inp4_ip_tos;
	xinp->inp_depend6.inp6_hlim = 0;
	xinp->inp_depend6.inp6_cksum = inp->inp_depend6.inp6_cksum;
	xinp->inp_depend6.inp6_ifindex = 0;
	xinp->inp_depend6.inp6_hops = inp->inp_depend6.inp6_hops;
}
#endif                        

  
                                                
  
                                                                          
                                                                          
                                                                          
                                                                           
                                                                          
                                                                           
                                                   
  
                                                                          
                                                                              
                                                                          
                                                                         
                                                            
  
                                                                         
                                                                             
                                                                             
                                                                            
                                                                             
                                                                          
                                                                           
                                  
   
void
inp_route_copyout(struct inpcb *inp, struct route *dst)
{
	struct route *src = &inp->inp_route;

	socket_lock_assert_owned(inp->inp_socket);

	  
                                                                   
                                                             
    
	if (ROUTE_UNUSABLE(src) || rt_key(src->ro_rt)->sa_family != AF_INET) {
		ROUTE_RELEASE(src);
	}

	route_copyout(dst, src, sizeof(*dst));
}

void
inp_route_copyin(struct inpcb *inp, struct route *src)
{
	struct route *dst = &inp->inp_route;

	socket_lock_assert_owned(inp->inp_socket);

	                        
	if (src->ro_rt != NULL && rt_key(src->ro_rt)->sa_family != AF_INET) {
		panic("%s: wrong or corrupted route: %p", __func__, src);
	}

	route_copyin(src, dst, sizeof(*src));
}

  
                                                               
   
int
inp_bindif(struct inpcb *inp, unsigned int ifscope, struct ifnet **pifp)
{
	struct ifnet *ifp = NULL;

	ifnet_head_lock_shared();
	if ((ifscope > (unsigned)if_index) || (ifscope != IFSCOPE_NONE &&
	    (ifp = ifindex2ifnet[ifscope]) == NULL)) {
		ifnet_head_done();
		return ENXIO;
	}
	ifnet_head_done();

	VERIFY(ifp != NULL || ifscope == IFSCOPE_NONE);

	  
                                                       
                                                      
                                                       
                                                       
                                                     
                                                   
                                                 
    
	inp->inp_boundifp = ifp;
	if (inp->inp_boundifp == NULL) {
		inp->inp_flags &= ~INP_BOUND_IF;
	} else {
		inp->inp_flags |= INP_BOUND_IF;
	}

	                                           
	ROUTE_RELEASE(&inp->inp_route);

	if (pifp != NULL) {
		*pifp = ifp;
	}

	return 0;
}

  
                                                                             
                                                       
   
void
inp_set_nocellular(struct inpcb *inp)
{
	inp->inp_flags |= INP_NO_IFT_CELLULAR;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

  
                                                                              
                                                        
   
void
inp_clear_nocellular(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

	  
                                                                     
                                                                      
                                              
    
	if (so != NULL && !(so->so_restrictions & SO_RESTRICT_DENY_CELLULAR)) {
		inp->inp_flags &= ~INP_NO_IFT_CELLULAR;

		                                           
		ROUTE_RELEASE(&inp->inp_route);
	}
}

void
inp_set_noexpensive(struct inpcb *inp)
{
	inp->inp_flags2 |= INP2_NO_IFF_EXPENSIVE;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

void
inp_set_noconstrained(struct inpcb *inp)
{
	inp->inp_flags2 |= INP2_NO_IFF_CONSTRAINED;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

void
inp_set_awdl_unrestricted(struct inpcb *inp)
{
	inp->inp_flags2 |= INP2_AWDL_UNRESTRICTED;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

boolean_t
inp_get_awdl_unrestricted(struct inpcb *inp)
{
	return (inp->inp_flags2 & INP2_AWDL_UNRESTRICTED) ? TRUE : FALSE;
}

void
inp_clear_awdl_unrestricted(struct inpcb *inp)
{
	inp->inp_flags2 &= ~INP2_AWDL_UNRESTRICTED;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

void
inp_set_intcoproc_allowed(struct inpcb *inp)
{
	inp->inp_flags2 |= INP2_INTCOPROC_ALLOWED;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

boolean_t
inp_get_intcoproc_allowed(struct inpcb *inp)
{
	return (inp->inp_flags2 & INP2_INTCOPROC_ALLOWED) ? TRUE : FALSE;
}

void
inp_clear_intcoproc_allowed(struct inpcb *inp)
{
	inp->inp_flags2 &= ~INP2_INTCOPROC_ALLOWED;

	                                           
	ROUTE_RELEASE(&inp->inp_route);
}

#if NECP
  
                                                
   
void
inp_set_want_app_policy(struct inpcb *inp)
{
	inp->inp_flags2 |= INP2_WANT_APP_POLICY;
}

  
                                                    
   
void
inp_clear_want_app_policy(struct inpcb *inp)
{
	inp->inp_flags2 &= ~INP2_WANT_APP_POLICY;
}
#endif           

  
                                                                     
                                                                    
                                 
   
u_int32_t
inp_calc_flowhash(struct inpcb *inp)
{
	struct inp_flowhash_key fh __attribute__((aligned(8)));
	u_int32_t flowhash = 0;
	struct inpcb *tmp_inp = NULL;

	if (inp_hash_seed == 0) {
		inp_hash_seed = RandomULong();
	}

	bzero(&fh, sizeof(fh));

	bcopy(&inp->inp_dependladdr, &fh.infh_laddr, sizeof(fh.infh_laddr));
	bcopy(&inp->inp_dependfaddr, &fh.infh_faddr, sizeof(fh.infh_faddr));

	fh.infh_lport = inp->inp_lport;
	fh.infh_fport = inp->inp_fport;
	fh.infh_af = (inp->inp_vflag & INP_IPV6) ? AF_INET6 : AF_INET;
	fh.infh_proto = inp->inp_ip_p;
	fh.infh_rand1 = RandomULong();
	fh.infh_rand2 = RandomULong();

try_again:
	flowhash = net_flowhash(&fh, sizeof(fh), inp_hash_seed);
	if (flowhash == 0) {
		                                    
		inp_hash_seed = RandomULong();
		goto try_again;
	}

	inp->inp_flowhash = flowhash;

	                                     
	lck_mtx_lock_spin(&inp_fc_lck);
	tmp_inp = RB_FIND(inp_fc_tree, &inp_fc_tree, inp);
	if (tmp_inp != NULL) {
		  
                                                     
                                                  
                                             
              
     
		lck_mtx_unlock(&inp_fc_lck);
		                         
		inp_hash_seed = RandomULong();
		goto try_again;
	}

	RB_INSERT(inp_fc_tree, &inp_fc_tree, inp);
	inp->inp_flags2 |= INP2_IN_FCTREE;
	lck_mtx_unlock(&inp_fc_lck);

	return flowhash;
}

void
inp_flowadv(uint32_t flowhash)
{
	struct inpcb *inp;

	inp = inp_fc_getinp(flowhash, 0);

	if (inp == NULL) {
		return;
	}
	inp_fc_feedback(inp);
}

  
                                                              
   
static inline int
infc_cmp(const struct inpcb *inp1, const struct inpcb *inp2)
{
	return memcmp(&(inp1->inp_flowhash), &(inp2->inp_flowhash),
	           sizeof(inp1->inp_flowhash));
}

static struct inpcb *
inp_fc_getinp(u_int32_t flowhash, u_int32_t flags)
{
	struct inpcb *inp = NULL;
	int locked = (flags & INPFC_SOLOCKED) ? 1 : 0;

	lck_mtx_lock_spin(&inp_fc_lck);
	key_inp.inp_flowhash = flowhash;
	inp = RB_FIND(inp_fc_tree, &inp_fc_tree, &key_inp);
	if (inp == NULL) {
		                                
		lck_mtx_unlock(&inp_fc_lck);
		return NULL;
	}

	if (flags & INPFC_REMOVE) {
		RB_REMOVE(inp_fc_tree, &inp_fc_tree, inp);
		lck_mtx_unlock(&inp_fc_lck);

		bzero(&(inp->infc_link), sizeof(inp->infc_link));
		inp->inp_flags2 &= ~INP2_IN_FCTREE;
		return NULL;
	}

	if (in_pcb_checkstate(inp, WNT_ACQUIRE, locked) == WNT_STOPUSING) {
		inp = NULL;
	}
	lck_mtx_unlock(&inp_fc_lck);

	return inp;
}

static void
inp_fc_feedback(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;

	                                                                  
	VERIFY(so != NULL);
	socket_lock(so, 1);

	if (in_pcb_checkstate(inp, WNT_RELEASE, 1) == WNT_STOPUSING) {
		socket_unlock(so, 1);
		return;
	}

	if (inp->inp_sndinprog_cnt > 0) {
		inp->inp_flags |= INP_FC_FEEDBACK;
	}

	  
                                                             
                                                 
                                              
    
	if (!INP_WAIT_FOR_IF_FEEDBACK(inp)) {
		socket_unlock(so, 1);
		return;
	}
	inp_reset_fc_state(inp);

	if (SOCK_TYPE(so) == SOCK_STREAM) {
		inp_fc_unthrottle_tcp(inp);
	}

	socket_unlock(so, 1);
}

void
inp_reset_fc_state(struct inpcb *inp)
{
	struct socket *so = inp->inp_socket;
	int suspended = (INP_IS_FLOW_SUSPENDED(inp)) ? 1 : 0;
	int needwakeup = (INP_WAIT_FOR_IF_FEEDBACK(inp)) ? 1 : 0;

	inp->inp_flags &= ~(INP_FLOW_CONTROLLED | INP_FLOW_SUSPENDED);

	if (suspended) {
		so->so_flags &= ~(SOF_SUSPENDED);
		soevent(so, (SO_FILT_HINT_LOCKED | SO_FILT_HINT_RESUME));
	}

	                                               
	if (needwakeup) {
		sowwakeup(so);
	}
}

int
inp_set_fc_state(struct inpcb *inp, int advcode)
{
	boolean_t is_flow_controlled = INP_WAIT_FOR_IF_FEEDBACK(inp);
	struct inpcb *tmp_inp = NULL;
	  
                                                   
                                                    
                                                      
                                                     
                 
    
	if (inp->inp_flags & INP_FC_FEEDBACK) {
		return 0;
	}

	inp->inp_flags &= ~(INP_FLOW_CONTROLLED | INP_FLOW_SUSPENDED);
	if ((tmp_inp = inp_fc_getinp(inp->inp_flowhash,
	    INPFC_SOLOCKED)) != NULL) {
		if (in_pcb_checkstate(tmp_inp, WNT_RELEASE, 1) == WNT_STOPUSING) {
			return 0;
		}
		VERIFY(tmp_inp == inp);
		switch (advcode) {
		case FADV_FLOW_CONTROLLED:
			inp->inp_flags |= INP_FLOW_CONTROLLED;
			break;
		case FADV_SUSPENDED:
			inp->inp_flags |= INP_FLOW_SUSPENDED;
			soevent(inp->inp_socket,
			    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_SUSPEND));

			                                                 
			inp->inp_socket->so_flags |= SOF_SUSPENDED;
			break;
		}

		if (!is_flow_controlled && SOCK_TYPE(inp->inp_socket) == SOCK_STREAM) {
			inp_fc_throttle_tcp(inp);
		}
		return 1;
	}
	return 0;
}

  
                                      
   
int
inp_flush(struct inpcb *inp, int optval)
{
	u_int32_t flowhash = inp->inp_flowhash;
	struct ifnet *rtifp, *oifp;

	                                                 
	if (optval != SO_TC_ALL && !SO_VALID_TC(optval)) {
		return EINVAL;
	}

	                                            
	if (flowhash == 0) {
		return 0;
	}

	                                                
	rtifp = ((inp->inp_route.ro_rt != NULL) ?
	    inp->inp_route.ro_rt->rt_ifp : NULL);
	oifp = inp->inp_last_outifp;

	if (rtifp != NULL) {
		if_qflush_sc(rtifp, so_tc2msc(optval), flowhash, NULL, NULL, 0);
	}
	if (oifp != NULL && oifp != rtifp) {
		if_qflush_sc(oifp, so_tc2msc(optval), flowhash, NULL, NULL, 0);
	}

	return 0;
}

  
                                                            
   
void
inp_clear_INP_INADDR_ANY(struct socket *so)
{
	struct inpcb *inp = NULL;

	socket_lock(so, 1);
	inp = sotoinpcb(so);
	if (inp) {
		inp->inp_flags &= ~INP_INADDR_ANY;
	}
	socket_unlock(so, 1);
}

void
inp_get_soprocinfo(struct inpcb *inp, struct so_procinfo *soprocinfo)
{
	struct socket *so = inp->inp_socket;

	soprocinfo->spi_pid = so->last_pid;
	strlcpy(&soprocinfo->spi_proc_name[0], &inp->inp_last_proc_name[0],
	    sizeof(soprocinfo->spi_proc_name));
	if (so->last_pid != 0) {
		uuid_copy(soprocinfo->spi_uuid, so->last_uuid);
	}
	  
                                                                     
    
	if (so->so_flags & SOF_DELEGATED) {
		soprocinfo->spi_delegated = 1;
		soprocinfo->spi_epid = so->e_pid;
		uuid_copy(soprocinfo->spi_euuid, so->e_uuid);
	} else {
		soprocinfo->spi_delegated = 0;
		soprocinfo->spi_epid = so->last_pid;
	}
	strlcpy(&soprocinfo->spi_e_proc_name[0], &inp->inp_e_proc_name[0],
	    sizeof(soprocinfo->spi_e_proc_name));
}

int
inp_findinpcb_procinfo(struct inpcbinfo *pcbinfo, uint32_t flowhash,
    struct so_procinfo *soprocinfo)
{
	struct inpcb *inp = NULL;
	int found = 0;

	bzero(soprocinfo, sizeof(struct so_procinfo));

	if (!flowhash) {
		return -1;
	}

	lck_rw_lock_shared(pcbinfo->ipi_lock);
	LIST_FOREACH(inp, pcbinfo->ipi_listhead, inp_list) {
		if (inp->inp_state != INPCB_STATE_DEAD &&
		    inp->inp_socket != NULL &&
		    inp->inp_flowhash == flowhash) {
			found = 1;
			inp_get_soprocinfo(inp, soprocinfo);
			break;
		}
	}
	lck_rw_done(pcbinfo->ipi_lock);

	return found;
}

#if CONFIG_PROC_UUID_POLICY
static void
inp_update_cellular_policy(struct inpcb *inp, boolean_t set)
{
	struct socket *so = inp->inp_socket;
	int before, after;

	VERIFY(so != NULL);
	VERIFY(inp->inp_state != INPCB_STATE_DEAD);

	before = INP_NO_CELLULAR(inp);
	if (set) {
		inp_set_nocellular(inp);
	} else {
		inp_clear_nocellular(inp);
	}
	after = INP_NO_CELLULAR(inp);
	if (net_io_policy_log && (before != after)) {
		static const char *ok = "OK";
		static const char *nok = "NOACCESS";
		uuid_string_t euuid_buf;
		pid_t epid;

		if (so->so_flags & SOF_DELEGATED) {
			uuid_unparse(so->e_uuid, euuid_buf);
			epid = so->e_pid;
		} else {
			uuid_unparse(so->last_uuid, euuid_buf);
			epid = so->last_pid;
		}

		                                                              
		so->so_ifdenied_notifies = 0;

		log(LOG_DEBUG, "%s: so 0x%llx [%d,%d] epid %d "
		    "euuid %s%s %s->%s\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), epid, euuid_buf,
		    (so->so_flags & SOF_DELEGATED) ?
		    " [delegated]" : "",
		    ((before < after) ? ok : nok),
		    ((before < after) ? nok : ok));
	}
}

#if NECP
static void
inp_update_necp_want_app_policy(struct inpcb *inp, boolean_t set)
{
	struct socket *so = inp->inp_socket;
	int before, after;

	VERIFY(so != NULL);
	VERIFY(inp->inp_state != INPCB_STATE_DEAD);

	before = (inp->inp_flags2 & INP2_WANT_APP_POLICY);
	if (set) {
		inp_set_want_app_policy(inp);
	} else {
		inp_clear_want_app_policy(inp);
	}
	after = (inp->inp_flags2 & INP2_WANT_APP_POLICY);
	if (net_io_policy_log && (before != after)) {
		static const char *wanted = "WANTED";
		static const char *unwanted = "UNWANTED";
		uuid_string_t euuid_buf;
		pid_t epid;

		if (so->so_flags & SOF_DELEGATED) {
			uuid_unparse(so->e_uuid, euuid_buf);
			epid = so->e_pid;
		} else {
			uuid_unparse(so->last_uuid, euuid_buf);
			epid = so->last_pid;
		}

		log(LOG_DEBUG, "%s: so 0x%llx [%d,%d] epid %d "
		    "euuid %s%s %s->%s\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), epid, euuid_buf,
		    (so->so_flags & SOF_DELEGATED) ?
		    " [delegated]" : "",
		    ((before < after) ? unwanted : wanted),
		    ((before < after) ? wanted : unwanted));
	}
}
#endif           
#endif                               

#if NECP
void
inp_update_necp_policy(struct inpcb *inp, struct sockaddr *override_local_addr, struct sockaddr *override_remote_addr, u_int override_bound_interface)
{
	necp_socket_find_policy_match(inp, override_local_addr, override_remote_addr, override_bound_interface);
	if (necp_socket_should_rescope(inp) &&
	    inp->inp_lport == 0 &&
	    inp->inp_laddr.s_addr == INADDR_ANY &&
	    IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
		                                                        
		inp_bindif(inp, necp_socket_get_rescope_if_index(inp), NULL);
	}
}
#endif           

int
inp_update_policy(struct inpcb *inp)
{
#if CONFIG_PROC_UUID_POLICY
	struct socket *so = inp->inp_socket;
	uint32_t pflags = 0;
	int32_t ogencnt;
	int err = 0;
	uint8_t *lookup_uuid = NULL;

	if (!net_io_policy_uuid ||
	    so == NULL || inp->inp_state == INPCB_STATE_DEAD) {
		return 0;
	}

	  
                                                               
                                                   
    
	if (so->last_pid == 0 && !(so->so_flags & SOF_DELEGATED)) {
		return 0;
	}

#if defined(XNU_TARGET_OS_OSX)
	if (so->so_rpid > 0) {
		lookup_uuid = so->so_ruuid;
		ogencnt = so->so_policy_gencnt;
		err = proc_uuid_policy_lookup(lookup_uuid, &pflags, &so->so_policy_gencnt);
	}
#endif
	if (lookup_uuid == NULL || err == ENOENT) {
		lookup_uuid = ((so->so_flags & SOF_DELEGATED) ? so->e_uuid : so->last_uuid);
		ogencnt = so->so_policy_gencnt;
		err = proc_uuid_policy_lookup(lookup_uuid, &pflags, &so->so_policy_gencnt);
	}

	  
                                                                  
                                        
    
	if (err == ENOENT && ogencnt != 0) {
		so->so_policy_gencnt = 0;
	}

	  
                                                                 
                                                                 
                                                                 
                                    
    
	if ((err == 0 || err == ENOENT) && ogencnt != so->so_policy_gencnt) {
		                                            
		if (err == 0 && (pflags & PROC_UUID_NO_CELLULAR)) {
			inp_update_cellular_policy(inp, TRUE);
		} else if (!(pflags & PROC_UUID_NO_CELLULAR)) {
			inp_update_cellular_policy(inp, FALSE);
		}
#if NECP
		                                                 
		if (err == 0 && (pflags & PROC_UUID_NECP_APP_POLICY)) {
			inp_update_necp_want_app_policy(inp, TRUE);
		} else if (!(pflags & PROC_UUID_NECP_APP_POLICY)) {
			inp_update_necp_want_app_policy(inp, FALSE);
		}
#endif           
	}

	return (err == ENOENT) ? 0 : err;
#else                               
#pragma unused(inp)
	return 0;
#endif                               
}

static unsigned int log_restricted;
SYSCTL_DECL(_net_inet);
SYSCTL_INT(_net_inet, OID_AUTO, log_restricted,
    CTLFLAG_RW | CTLFLAG_LOCKED, &log_restricted, 0,
    "Log network restrictions");
  
                                                                        
  
                                                                      
   
static boolean_t
_inp_restricted_recv(struct inpcb *inp, struct ifnet *ifp)
{
	VERIFY(inp != NULL);

	  
                         
    
	if (!sorestrictrecv) {
		return FALSE;
	}

	if (ifp == NULL) {
		return FALSE;
	}

	if (IFNET_IS_CELLULAR(ifp) && INP_NO_CELLULAR(inp)) {
		return TRUE;
	}

	if (IFNET_IS_EXPENSIVE(ifp) && INP_NO_EXPENSIVE(inp)) {
		return TRUE;
	}

	if (IFNET_IS_CONSTRAINED(ifp) && INP_NO_CONSTRAINED(inp)) {
		return TRUE;
	}

	if (IFNET_IS_AWDL_RESTRICTED(ifp) && !INP_AWDL_UNRESTRICTED(inp)) {
		return TRUE;
	}

	if (!(ifp->if_eflags & IFEF_RESTRICTED_RECV)) {
		return FALSE;
	}

	if (inp->inp_flags & INP_RECV_ANYIF) {
		return FALSE;
	}

	if ((inp->inp_flags & INP_BOUND_IF) && inp->inp_boundifp == ifp) {
		return FALSE;
	}

	if (IFNET_IS_INTCOPROC(ifp) && !INP_INTCOPROC_ALLOWED(inp)) {
		return TRUE;
	}

	return TRUE;
}

boolean_t
inp_restricted_recv(struct inpcb *inp, struct ifnet *ifp)
{
	boolean_t ret;

	ret = _inp_restricted_recv(inp, ifp);
	if (ret == TRUE && log_restricted) {
		printf("pid %d (%s) is unable to receive packets on %s\n",
		    current_proc()->p_pid, proc_best_name(current_proc()),
		    ifp->if_xname);
	}
	return ret;
}

  
                                                                         
  
                                                                       
   
static boolean_t
_inp_restricted_send(struct inpcb *inp, struct ifnet *ifp)
{
	VERIFY(inp != NULL);

	  
                          
    
	if (!sorestrictsend) {
		return FALSE;
	}

	if (ifp == NULL) {
		return FALSE;
	}

	if (IFNET_IS_CELLULAR(ifp) && INP_NO_CELLULAR(inp)) {
		return TRUE;
	}

	if (IFNET_IS_EXPENSIVE(ifp) && INP_NO_EXPENSIVE(inp)) {
		return TRUE;
	}

	if (IFNET_IS_CONSTRAINED(ifp) && INP_NO_CONSTRAINED(inp)) {
		return TRUE;
	}

	if (IFNET_IS_AWDL_RESTRICTED(ifp) && !INP_AWDL_UNRESTRICTED(inp)) {
		return TRUE;
	}

	if (IFNET_IS_INTCOPROC(ifp) && !INP_INTCOPROC_ALLOWED(inp)) {
		return TRUE;
	}

	return FALSE;
}

boolean_t
inp_restricted_send(struct inpcb *inp, struct ifnet *ifp)
{
	boolean_t ret;

	ret = _inp_restricted_send(inp, ifp);
	if (ret == TRUE && log_restricted) {
		printf("pid %d (%s) is unable to transmit packets on %s\n",
		    current_proc()->p_pid, proc_best_name(current_proc()),
		    ifp->if_xname);
	}
	return ret;
}

inline void
inp_count_sndbytes(struct inpcb *inp, u_int32_t th_ack)
{
	struct ifnet *ifp = inp->inp_last_outifp;
	struct socket *so = inp->inp_socket;
	if (ifp != NULL && !(so->so_flags & SOF_MP_SUBFLOW) &&
	    (ifp->if_type == IFT_CELLULAR || IFNET_IS_WIFI(ifp))) {
		int32_t unsent;

		so->so_snd.sb_flags |= SB_SNDBYTE_CNT;

		  
                                                        
                                    
     
		if (so->so_snd.sb_cc > 0) {
			inp_incr_sndbytes_total(so, so->so_snd.sb_cc);
		}

		unsent = inp_get_sndbytes_allunsent(so, th_ack);
		if (unsent > 0) {
			inp_incr_sndbytes_unsent(so, unsent);
		}
	}
}

inline void
inp_incr_sndbytes_total(struct socket *so, int32_t len)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;
	struct ifnet *ifp = inp->inp_last_outifp;

	if (ifp != NULL) {
		VERIFY(ifp->if_sndbyte_total >= 0);
		OSAddAtomic64(len, &ifp->if_sndbyte_total);
	}
}

inline void
inp_decr_sndbytes_total(struct socket *so, int32_t len)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;
	struct ifnet *ifp = inp->inp_last_outifp;

	if (ifp != NULL) {
		VERIFY(ifp->if_sndbyte_total >= len);
		OSAddAtomic64(-len, &ifp->if_sndbyte_total);
	}
}

inline void
inp_incr_sndbytes_unsent(struct socket *so, int32_t len)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;
	struct ifnet *ifp = inp->inp_last_outifp;

	if (ifp != NULL) {
		VERIFY(ifp->if_sndbyte_unsent >= 0);
		OSAddAtomic64(len, &ifp->if_sndbyte_unsent);
	}
}

inline void
inp_decr_sndbytes_unsent(struct socket *so, int32_t len)
{
	if (so == NULL || !(so->so_snd.sb_flags & SB_SNDBYTE_CNT)) {
		return;
	}

	struct inpcb *inp = (struct inpcb *)so->so_pcb;
	struct ifnet *ifp = inp->inp_last_outifp;

	if (ifp != NULL) {
		if (ifp->if_sndbyte_unsent >= len) {
			OSAddAtomic64(-len, &ifp->if_sndbyte_unsent);
		} else {
			ifp->if_sndbyte_unsent = 0;
		}
	}
}

inline void
inp_decr_sndbytes_allunsent(struct socket *so, u_int32_t th_ack)
{
	int32_t len;

	if (so == NULL || !(so->so_snd.sb_flags & SB_SNDBYTE_CNT)) {
		return;
	}

	len = inp_get_sndbytes_allunsent(so, th_ack);
	inp_decr_sndbytes_unsent(so, len);
}


inline void
inp_set_activity_bitmap(struct inpcb *inp)
{
	in_stat_set_activity_bitmap(&inp->inp_nw_activity, net_uptime());
}

inline void
inp_get_activity_bitmap(struct inpcb *inp, activity_bitmap_t *ab)
{
	bcopy(&inp->inp_nw_activity, ab, sizeof(*ab));
}

void
inp_update_last_owner(struct socket *so, struct proc *p, struct proc *ep)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;

	if (inp == NULL) {
		return;
	}

	if (p != NULL) {
		strlcpy(&inp->inp_last_proc_name[0], proc_name_address(p), sizeof(inp->inp_last_proc_name));
	}
	if (so->so_flags & SOF_DELEGATED) {
		if (ep != NULL) {
			strlcpy(&inp->inp_e_proc_name[0], proc_name_address(ep), sizeof(inp->inp_e_proc_name));
		} else {
			inp->inp_e_proc_name[0] = 0;
		}
	} else {
		inp->inp_e_proc_name[0] = 0;
	}
}

void
inp_copy_last_owner(struct socket *so, struct socket *head)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;
	struct inpcb *head_inp = (struct inpcb *)head->so_pcb;

	if (inp == NULL || head_inp == NULL) {
		return;
	}

	strlcpy(&inp->inp_last_proc_name[0], &head_inp->inp_last_proc_name[0], sizeof(inp->inp_last_proc_name));
	strlcpy(&inp->inp_e_proc_name[0], &head_inp->inp_e_proc_name[0], sizeof(inp->inp_e_proc_name));
}
