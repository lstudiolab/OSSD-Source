/*
 * Copyright (c) 2011-2020 Apple Inc. All rights reserved.
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/kauth.h>

#include <kern/zalloc.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/dlil.h>
#include <net/flowadv.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <net/classq/classq_sfb.h>
#include <net/flowhash.h>
#include <net/net_osdep.h>
#include <dev/random/randomdev.h>

  
                       
  
                                                               
                                                         
  
                                                      
  
                 
                     
                     
                                      
                 
            
                                     
                                  
  
                                                           
  
                                                            
  
                                                                       
                                   
  
                                                                         
                                                                           
                                                                            
                                                                            
                                                                
   

  
                                                                           
                                                                           
                                                            
   
#define SFB_HASH        net_flowhash_mh3_x86_32
#define SFB_HASHMASK    HASHMASK(16)

#define SFB_BINMASK(_x) \
	((_x) & HASHMASK(SFB_BINS_SHIFT))

#define SFB_BINST(_sp, _l, _n, _c) \
	(&(*(_sp)->sfb_bins)[_c].stats[_l][_n])

#define SFB_BINFT(_sp, _l, _n, _c) \
	(&(*(_sp)->sfb_bins)[_c].freezetime[_l][_n])

#define SFB_FC_LIST(_sp, _n) \
	(&(*(_sp)->sfb_fc_lists)[_n])

  
                                                                      
                                                                       
                                                                         
                                    
   
#define HOLDTIME_BASE   (100ULL * 1000 * 1000)             
#define HOLDTIME_MIN    (10ULL * 1000 * 1000)             
#define HOLDTIME_MAX    (100ULL * 1000 * 1000)             

  
                                                                      
                                                                          
                                                                         
                       
   
#define PBOXTIME_BASE   (300ULL * 1000 * 1000)             
#define PBOXTIME_MIN    (30ULL * 1000 * 1000)             
#define PBOXTIME_MAX    (300ULL * 1000 * 1000)             

  
                                                                       
                                                                       
                                                                     
   
#define TARGET_QDELAY_BASE      (10ULL * 1000 * 1000)             
#define TARGET_QDELAY_MIN       (10ULL * 1000)            
#define TARGET_QDELAY_MAX       (20ULL * 1000 * 1000 * 1000)             

  
                                                                        
                                                                      
                                                  
   
#define UPDATE_INTERVAL_BASE    (100ULL * 1000 * 1000)             
#define UPDATE_INTERVAL_MIN     (100ULL * 1000 * 1000)             
#define UPDATE_INTERVAL_MAX     (10ULL * 1000 * 1000 * 1000)             

#define SFB_RANDOM(sp, tmin, tmax)      ((sfb_random(sp) % (tmax)) + (tmin))

#define SFB_PKT_PBOX    0x1                                 

                                                                
#define SFB_MAX_PMARK   (1 << SFB_FP_SHIFT)                                 

  
                                                                            
                                                                            
                                                                             
                                                                            
                                                                         
                                                                       
                                                                         
                
   
#define SFB_INCREMENT   82                                               
#define SFB_DECREMENT   16                                               

#define SFB_PMARK_TH    16056                                           
#define SFB_PMARK_WARM  3276                                           

#define SFB_PMARK_INC(_bin) do {                                        \
	(_bin)->pmark += sfb_increment;                                 \
	if ((_bin)->pmark > SFB_MAX_PMARK)                              \
	        (_bin)->pmark = SFB_MAX_PMARK;                          \
} while (0)

#define SFB_PMARK_DEC(_bin) do {                                        \
	if ((_bin)->pmark > 0) {                                        \
	        (_bin)->pmark -= sfb_decrement;                         \
	        if ((_bin)->pmark < 0)                                  \
	                (_bin)->pmark = 0;                              \
	}                                                               \
} while (0)

                                                            
#define SFB_MIN_FC_THRESHOLD_BYTES      7500

#define SFB_SET_DELAY_HIGH(_sp_, _q_) do {                              \
	(_sp_)->sfb_flags |= SFBF_DELAYHIGH;                            \
	(_sp_)->sfb_fc_threshold = ulmax(SFB_MIN_FC_THRESHOLD_BYTES,    \
	                                 (uint32_t)(qsize((_q_)) >> 3)); \
} while (0)

#define SFB_QUEUE_DELAYBASED(_sp_) ((_sp_)->sfb_flags & SFBF_DELAYBASED)
#define SFB_IS_DELAYHIGH(_sp_) ((_sp_)->sfb_flags & SFBF_DELAYHIGH)
#define SFB_QUEUE_DELAYBASED_MAXSIZE    2048                  

#define HINTERVAL_MIN   (10)                    
#define HINTERVAL_MAX   (20)                    
#define SFB_HINTERVAL(sp) ((sfb_random(sp) % HINTERVAL_MAX) + HINTERVAL_MIN)

#define DEQUEUE_DECAY   7                                                    
#define DEQUEUE_SPIKE(_new, _old)       \
	((u_int64_t)ABS((int64_t)(_new) - (int64_t)(_old)) > ((_old) << 11))

                                                              
#define SFB_FC_LEVEL    0

static ZONE_DECLARE(sfb_zone, "classq_sfb",
    sizeof(struct sfb), ZC_ZFREE_CLEARMEM);

static ZONE_DECLARE(sfb_bins_zone, "classq_sfb_bins",
    sizeof(struct sfb_bins), ZC_ZFREE_CLEARMEM);

static ZONE_DECLARE(sfb_fcl_zone, "classq_sfb_fcl",
    sizeof(struct sfb_fcl), ZC_ZFREE_CLEARMEM);

                                  
static u_int32_t sfb_random(struct sfb *);
static void *sfb_getq_flow(struct sfb *, class_queue_t *, u_int32_t, boolean_t,
    pktsched_pkt_t *);
static void sfb_resetq(struct sfb *, cqev_t);
static void sfb_calc_holdtime(struct sfb *, u_int64_t);
static void sfb_calc_pboxtime(struct sfb *, u_int64_t);
static void sfb_calc_hinterval(struct sfb *, u_int64_t *);
static void sfb_calc_update_interval(struct sfb *, u_int64_t);
static void sfb_swap_bins(struct sfb *, u_int32_t);
static inline int sfb_pcheck(struct sfb *, uint32_t);
static int sfb_penalize(struct sfb *, uint32_t, uint32_t *, struct timespec *);
static void sfb_adjust_bin(struct sfb *, struct sfbbinstats *,
    struct timespec *, struct timespec *, boolean_t);
static void sfb_decrement_bin(struct sfb *, struct sfbbinstats *,
    struct timespec *, struct timespec *);
static void sfb_increment_bin(struct sfb *, struct sfbbinstats *,
    struct timespec *, struct timespec *);
static inline void sfb_dq_update_bins(struct sfb *, uint32_t, uint32_t,
    struct timespec *, u_int64_t qsize);
static inline void sfb_eq_update_bins(struct sfb *, uint32_t, uint32_t);
static int sfb_drop_early(struct sfb *, uint32_t, u_int16_t *,
    struct timespec *);
static boolean_t sfb_bin_addfcentry(struct sfb *, pktsched_pkt_t *,
    uint32_t, uint8_t, uint32_t);
static void sfb_fclist_append(struct sfb *, struct sfb_fcl *);
static void sfb_fclists_clean(struct sfb *sp);
static int sfb_bin_mark_or_drop(struct sfb *sp, struct sfbbinstats *bin);
static void sfb_detect_dequeue_stall(struct sfb *sp, class_queue_t *,
    struct timespec *);

SYSCTL_NODE(_net_classq, OID_AUTO, sfb, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "SFB");

static u_int64_t sfb_holdtime = 0;                                   
SYSCTL_QUAD(_net_classq_sfb, OID_AUTO, holdtime, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_holdtime, "SFB freeze time in nanoseconds");

static u_int64_t sfb_pboxtime = 0;                                   
SYSCTL_QUAD(_net_classq_sfb, OID_AUTO, pboxtime, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_pboxtime, "SFB penalty box time in nanoseconds");

static u_int64_t sfb_hinterval;
SYSCTL_QUAD(_net_classq_sfb, OID_AUTO, hinterval, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_hinterval, "SFB hash interval in nanoseconds");

static u_int32_t sfb_increment = SFB_INCREMENT;
SYSCTL_UINT(_net_classq_sfb, OID_AUTO, increment, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_increment, SFB_INCREMENT, "SFB increment [d1]");

static u_int32_t sfb_decrement = SFB_DECREMENT;
SYSCTL_UINT(_net_classq_sfb, OID_AUTO, decrement, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_decrement, SFB_DECREMENT, "SFB decrement [d2]");

static u_int32_t sfb_allocation = 0;                             
SYSCTL_UINT(_net_classq_sfb, OID_AUTO, allocation, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_allocation, 0, "SFB bin allocation");

static u_int32_t sfb_ratelimit = 0;
SYSCTL_UINT(_net_classq_sfb, OID_AUTO, ratelimit, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sfb_ratelimit, 0, "SFB rate limit");

#define KBPS    (1ULL * 1000)                                   
#define MBPS    (1ULL * 1000 * 1000)                            
#define GBPS    (MBPS * 1000)                                   

struct sfb_time_tbl {
	u_int64_t       speed;                            
	u_int64_t       holdtime;                      
	u_int64_t       pboxtime;                             
};

static struct sfb_time_tbl sfb_ttbl[] = {
	{ .speed =   1 * MBPS, .holdtime = HOLDTIME_BASE * 1000, .pboxtime = PBOXTIME_BASE * 1000},
	{ .speed =  10 * MBPS, .holdtime = HOLDTIME_BASE * 100, .pboxtime = PBOXTIME_BASE * 100 },
	{ .speed = 100 * MBPS, .holdtime = HOLDTIME_BASE * 10, .pboxtime = PBOXTIME_BASE * 10  },
	{ .speed =   1 * GBPS, .holdtime = HOLDTIME_BASE, .pboxtime = PBOXTIME_BASE       },
	{ .speed =  10 * GBPS, .holdtime = HOLDTIME_BASE / 10, .pboxtime = PBOXTIME_BASE / 10  },
	{ .speed = 100 * GBPS, .holdtime = HOLDTIME_BASE / 100, .pboxtime = PBOXTIME_BASE / 100 },
	{ .speed = 0, .holdtime = 0, .pboxtime = 0 }
};

static_assert(SFBF_ECN4 == CLASSQF_ECN4);
static_assert(SFBF_ECN6 == CLASSQF_ECN6);

static u_int32_t
sfb_random(struct sfb *sp)
{
	IFCQ_CONVERT_LOCK(&sp->sfb_ifp->if_snd);
	return RandomULong();
}

static void
sfb_calc_holdtime(struct sfb *sp, u_int64_t outbw)
{
	u_int64_t holdtime;

	if (sfb_holdtime != 0) {
		holdtime = sfb_holdtime;
	} else if (outbw == 0) {
		holdtime = SFB_RANDOM(sp, HOLDTIME_MIN, HOLDTIME_MAX);
	} else {
		uint64_t n, i;

		n = sfb_ttbl[0].holdtime;
		for (i = 0; sfb_ttbl[i].speed != 0; i++) {
			if (outbw < sfb_ttbl[i].speed) {
				break;
			}
			n = sfb_ttbl[i].holdtime;
		}
		holdtime = n;
	}
	net_nsectimer(&holdtime, &sp->sfb_holdtime);
}

static void
sfb_calc_pboxtime(struct sfb *sp, u_int64_t outbw)
{
	u_int64_t pboxtime;

	if (sfb_pboxtime != 0) {
		pboxtime = sfb_pboxtime;
	} else if (outbw == 0) {
		pboxtime = SFB_RANDOM(sp, PBOXTIME_MIN, PBOXTIME_MAX);
	} else {
		uint64_t n, i;

		n = sfb_ttbl[0].pboxtime;
		for (i = 0; sfb_ttbl[i].speed != 0; i++) {
			if (outbw < sfb_ttbl[i].speed) {
				break;
			}
			n = sfb_ttbl[i].pboxtime;
		}
		pboxtime = n;
	}
	net_nsectimer(&pboxtime, &sp->sfb_pboxtime);
	net_timerclear(&sp->sfb_pboxfreeze);
}

static void
sfb_calc_hinterval(struct sfb *sp, u_int64_t *t)
{
	u_int64_t hinterval = 0;
	struct timespec now;

	if (t != NULL) {
		  
                                                        
     
		hinterval = *t;
	}

	if (sfb_hinterval != 0) {
		hinterval = sfb_hinterval;
	} else if (t == NULL || hinterval == 0) {
		hinterval = ((u_int64_t)SFB_HINTERVAL(sp) * NSEC_PER_SEC);
	}

	net_nsectimer(&hinterval, &sp->sfb_hinterval);

	nanouptime(&now);
	net_timeradd(&now, &sp->sfb_hinterval, &sp->sfb_nextreset);
}

static void
sfb_calc_update_interval(struct sfb *sp, u_int64_t out_bw)
{
#pragma unused(out_bw)
	u_int64_t update_interval = 0;
	ifclassq_calc_update_interval(&update_interval);
	net_nsectimer(&update_interval, &sp->sfb_update_interval);
}

  
                       
   
struct sfb *
sfb_alloc(struct ifnet *ifp, u_int32_t qid, u_int32_t qlim, u_int32_t flags)
{
	struct sfb *sp;
	int i;

	VERIFY(ifp != NULL && qlim > 0);

	sp = zalloc_flags(sfb_zone, Z_WAITOK | Z_ZERO);
	sp->sfb_bins = zalloc_flags(sfb_bins_zone, Z_WAITOK | Z_ZERO);
	sp->sfb_fc_lists = zalloc_flags(sfb_fcl_zone, Z_WAITOK | Z_ZERO);

	for (i = 0; i < SFB_BINS; ++i) {
		STAILQ_INIT(&SFB_FC_LIST(sp, i)->fclist);
	}

	sp->sfb_ifp = ifp;
	sp->sfb_qlim = qlim;
	sp->sfb_qid = qid;
	sp->sfb_flags = (flags & SFBF_USERFLAGS);
#if !PF_ECN
	if (sp->sfb_flags & SFBF_ECN) {
		sp->sfb_flags &= ~SFBF_ECN;
		log(LOG_ERR, "%s: SFB qid=%d, ECN not available; ignoring "
		    "SFBF_ECN flag!\n", if_name(ifp), sp->sfb_qid);
	}
#endif              

	sfb_resetq(sp, CLASSQ_EV_INIT);

	return sp;
}

static void
sfb_fclist_append(struct sfb *sp, struct sfb_fcl *fcl)
{
	IFCQ_CONVERT_LOCK(&sp->sfb_ifp->if_snd);
	VERIFY(STAILQ_EMPTY(&fcl->fclist) || fcl->cnt > 0);
	sp->sfb_stats.flow_feedback += fcl->cnt;
	fcl->cnt = 0;

	flowadv_add(&fcl->fclist);
	VERIFY(fcl->cnt == 0 && STAILQ_EMPTY(&fcl->fclist));
}

static void
sfb_fclists_clean(struct sfb *sp)
{
	int i;

	                                                           
	for (i = 0; i < SFB_BINS; ++i) {
		struct sfb_fcl *fcl = SFB_FC_LIST(sp, i);
		if (!STAILQ_EMPTY(&fcl->fclist)) {
			sfb_fclist_append(sp, fcl);
		}
	}
}

void
sfb_destroy(struct sfb *sp)
{
	sfb_fclists_clean(sp);
	if (sp->sfb_bins != NULL) {
		zfree(sfb_bins_zone, sp->sfb_bins);
		sp->sfb_bins = NULL;
	}
	if (sp->sfb_fc_lists != NULL) {
		zfree(sfb_fcl_zone, sp->sfb_fc_lists);
		sp->sfb_fc_lists = NULL;
	}
	zfree(sfb_zone, sp);
}

static void
sfb_resetq(struct sfb *sp, cqev_t ev)
{
	struct ifnet *ifp = sp->sfb_ifp;
	u_int64_t eff_rate;

	VERIFY(ifp != NULL);

	if (ev != CLASSQ_EV_LINK_DOWN) {
		(*sp->sfb_bins)[0].fudge = sfb_random(sp);
		(*sp->sfb_bins)[1].fudge = sfb_random(sp);
		sp->sfb_allocation = sfb_allocation == 0 ?
		    (uint16_t)(sp->sfb_qlim / 3) :
		    (uint16_t)sfb_allocation;
		sp->sfb_drop_thresh = sp->sfb_allocation +
		    (sp->sfb_allocation >> 1);
	}

	sp->sfb_clearpkts = 0;
	sp->sfb_current = 0;

	eff_rate = ifnet_output_linkrate(ifp);
	sp->sfb_eff_rate = eff_rate;

	sfb_calc_holdtime(sp, eff_rate);
	sfb_calc_pboxtime(sp, eff_rate);
	sfb_calc_hinterval(sp, NULL);
	ifclassq_calc_target_qdelay(ifp, &sp->sfb_target_qdelay);
	sfb_calc_update_interval(sp, eff_rate);

	if (ev == CLASSQ_EV_LINK_DOWN ||
	    ev == CLASSQ_EV_LINK_UP) {
		sfb_fclists_clean(sp);
	}

	bzero(sp->sfb_bins, sizeof(*sp->sfb_bins));
	bzero(&sp->sfb_stats, sizeof(sp->sfb_stats));

	if (ev == CLASSQ_EV_LINK_DOWN || !classq_verbose) {
		return;
	}

	log(LOG_DEBUG, "%s: SFB qid=%d, holdtime=%llu nsec, "
	    "pboxtime=%llu nsec, allocation=%d, drop_thresh=%d, "
	    "hinterval=%d sec, sfb_bins=%d bytes, eff_rate=%llu bps"
	    "target_qdelay= %llu nsec "
	    "update_interval=%llu sec %llu nsec flags=0x%x\n",
	    if_name(ifp), sp->sfb_qid, (u_int64_t)sp->sfb_holdtime.tv_nsec,
	    (u_int64_t)sp->sfb_pboxtime.tv_nsec,
	    (u_int32_t)sp->sfb_allocation, (u_int32_t)sp->sfb_drop_thresh,
	    (int)sp->sfb_hinterval.tv_sec, (int)sizeof(*sp->sfb_bins),
	    eff_rate, (u_int64_t)sp->sfb_target_qdelay,
	    (u_int64_t)sp->sfb_update_interval.tv_sec,
	    (u_int64_t)sp->sfb_update_interval.tv_nsec, sp->sfb_flags);
}

void
sfb_getstats(struct sfb *sp, struct sfb_stats *sps)
{
	sps->allocation = sp->sfb_allocation;
	sps->dropthresh = sp->sfb_drop_thresh;
	sps->clearpkts = sp->sfb_clearpkts;
	sps->current = sp->sfb_current;
	sps->target_qdelay = sp->sfb_target_qdelay;
	sps->min_estdelay = sp->sfb_min_qdelay;
	sps->delay_fcthreshold = (uint32_t)sp->sfb_fc_threshold;
	sps->flags = sp->sfb_flags;

	net_timernsec(&sp->sfb_holdtime, &sp->sfb_stats.hold_time);
	net_timernsec(&sp->sfb_pboxtime, &sp->sfb_stats.pbox_time);
	net_timernsec(&sp->sfb_hinterval, &sp->sfb_stats.rehash_intval);
	net_timernsec(&sp->sfb_update_interval, &sps->update_interval);
	*(&(sps->sfbstats)) = *(&(sp->sfb_stats));

	_CASSERT(sizeof((*sp->sfb_bins)[0].stats) ==
	    sizeof(sps->binstats[0].stats));

	bcopy(&(*sp->sfb_bins)[0].stats, &sps->binstats[0].stats,
	    sizeof(sps->binstats[0].stats));
	bcopy(&(*sp->sfb_bins)[1].stats, &sps->binstats[1].stats,
	    sizeof(sps->binstats[1].stats));
}

static void
sfb_swap_bins(struct sfb *sp, u_int32_t len)
{
	int i, j, s;

	if (sp->sfb_flags & SFBF_SUSPENDED) {
		return;
	}

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	(*sp->sfb_bins)[s].fudge = sfb_random(sp);                             
	sp->sfb_clearpkts = len;
	sp->sfb_stats.num_rehash++;

	s = (sp->sfb_current ^= 1);                                      

	if (classq_verbose) {
		log(LOG_DEBUG, "%s: SFB qid=%d, set %d is now current, "
		    "qlen=%d\n", if_name(sp->sfb_ifp), sp->sfb_qid, s, len);
	}

	                                           
	bzero(&(*sp->sfb_bins)[s].freezetime,
	    sizeof((*sp->sfb_bins)[s].freezetime));

	                                                        
	for (i = 0; i < SFB_BINS; i++) {
		struct sfb_fcl *fcl = SFB_FC_LIST(sp, i);

		if (!STAILQ_EMPTY(&fcl->fclist)) {
			sfb_fclist_append(sp, fcl);
		}

		for (j = 0; j < SFB_LEVELS; j++) {
			struct sfbbinstats *cbin, *wbin;

			cbin = SFB_BINST(sp, j, i, s);                       
			wbin = SFB_BINST(sp, j, i, s ^ 1);                   

			cbin->pkts = 0;
			cbin->bytes = 0;
			if (cbin->pmark > SFB_MAX_PMARK) {
				cbin->pmark = SFB_MAX_PMARK;
			}
			if (cbin->pmark < 0) {
				cbin->pmark = 0;
			}

			  
                                        
                                  
      
			if (wbin->pmark > SFB_PMARK_WARM) {
				wbin->pmark = SFB_PMARK_WARM;
			}
		}
	}
}

static inline int
sfb_pcheck(struct sfb *sp, uint32_t pkt_sfb_hash)
{
#if SFB_LEVELS != 2
	int i, n;
#endif                      
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;
	int s;

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	  
                                                             
                                           
    
#if SFB_LEVELS == 2
	  
                                                      
                                                      
    
	if (SFB_BINST(sp, 0, SFB_BINMASK(pkt_sfb_hash8[(s << 1)]),
	    s)->pmark < SFB_PMARK_TH ||
	    SFB_BINST(sp, 1, SFB_BINMASK(pkt_sfb_hash8[(s << 1) + 1]),
	    s)->pmark < SFB_PMARK_TH) {
		return 0;
	}
#else                      
	for (i = 0; i < SFB_LEVELS; i++) {
		if (s == 0) {                                       
			n = SFB_BINMASK(pkt_sfb_hash8[i]);
		} else {                                            
			n = SFB_BINMASK(pkt_sfb_hash8[i + 2]);
		}

		if (SFB_BINST(sp, i, n, s)->pmark < SFB_PMARK_TH) {
			return 0;
		}
	}
#endif                      
	return 1;
}

static int
sfb_penalize(struct sfb *sp, uint32_t pkt_sfb_hash, uint32_t *pkt_sfb_flags,
    struct timespec *now)
{
	struct timespec delta = { .tv_sec = 0, .tv_nsec = 0 };
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;

	                                                                    
	if (!sfb_ratelimit || !sfb_pcheck(sp, pkt_sfb_hash)) {
		return 0;
	}

	net_timersub(now, &sp->sfb_pboxfreeze, &delta);
	if (net_timercmp(&delta, &sp->sfb_pboxtime, <)) {
#if SFB_LEVELS != 2
		int i;
#endif                      
		struct sfbbinstats *bin;
		int n, w;

		w = sp->sfb_current ^ 1;
		VERIFY((w + (w ^ 1)) == 1);

		  
                                                   
     
#if SFB_LEVELS == 2
		                                                        
		n = SFB_BINMASK(pkt_sfb_hash8[(w << 1)]);
		bin = SFB_BINST(sp, 0, n, w);
		if (bin->pkts >= sp->sfb_allocation) {
			sfb_increment_bin(sp, bin, SFB_BINFT(sp, 0, n, w), now);
		}

		                                                        
		n = SFB_BINMASK(pkt_sfb_hash8[(w << 1) + 1]);
		bin = SFB_BINST(sp, 1, n, w);
		if (bin->pkts >= sp->sfb_allocation) {
			sfb_increment_bin(sp, bin, SFB_BINFT(sp, 1, n, w), now);
		}
#else                      
		for (i = 0; i < SFB_LEVELS; i++) {
			if (w == 0) {                               
				n = SFB_BINMASK(pkt_sfb_hash8[i]);
			} else {                                    
				n = SFB_BINMASK(pkt_sfb_hash8[i + 2]);
			}

			bin = SFB_BINST(sp, i, n, w);
			if (bin->pkts >= sp->sfb_allocation) {
				sfb_increment_bin(sp, bin,
				    SFB_BINFT(sp, i, n, w), now);
			}
		}
#endif                      
		return 1;
	}

	                                                                
	*pkt_sfb_flags |= SFB_PKT_PBOX;
	*(&sp->sfb_pboxfreeze) = *now;

	return 0;
}

static void
sfb_adjust_bin(struct sfb *sp, struct sfbbinstats *bin, struct timespec *ft,
    struct timespec *now, boolean_t inc)
{
	struct timespec delta;

	net_timersub(now, ft, &delta);
	if (net_timercmp(&delta, &sp->sfb_holdtime, <)) {
		if (classq_verbose > 1) {
			log(LOG_DEBUG, "%s: SFB qid=%d, %s update frozen "
			    "(delta=%llu nsec)\n", if_name(sp->sfb_ifp),
			    sp->sfb_qid, inc ?  "increment" : "decrement",
			    (u_int64_t)delta.tv_nsec);
		}
		return;
	}

	                                             
	*ft = *now;
	if (inc) {
		SFB_PMARK_INC(bin);
	} else {
		SFB_PMARK_DEC(bin);
	}
}

static void
sfb_decrement_bin(struct sfb *sp, struct sfbbinstats *bin, struct timespec *ft,
    struct timespec *now)
{
	return sfb_adjust_bin(sp, bin, ft, now, FALSE);
}

static void
sfb_increment_bin(struct sfb *sp, struct sfbbinstats *bin, struct timespec *ft,
    struct timespec *now)
{
	return sfb_adjust_bin(sp, bin, ft, now, TRUE);
}

static inline void
sfb_dq_update_bins(struct sfb *sp, uint32_t pkt_sfb_hash, uint32_t pkt_len,
    struct timespec *now, u_int64_t qsize)
{
#if SFB_LEVELS != 2 || SFB_FC_LEVEL != 0
	int i;
#endif                                           
	struct sfbbinstats *bin;
	int s, n;
	struct sfb_fcl *fcl = NULL;
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	  
                                                                     
    
#if SFB_LEVELS == 2 && SFB_FC_LEVEL == 0
	                                                        
	n = SFB_BINMASK(pkt_sfb_hash8[(s << 1)]);
	bin = SFB_BINST(sp, 0, n, s);

	VERIFY(bin->pkts > 0 && bin->bytes >= pkt_len);
	bin->pkts--;
	bin->bytes -= pkt_len;

	if (bin->pkts == 0) {
		sfb_decrement_bin(sp, bin, SFB_BINFT(sp, 0, n, s), now);
	}

	                                                  
	if (SFB_QUEUE_DELAYBASED(sp)) {
		if (!(SFB_IS_DELAYHIGH(sp)) ||
		    bin->bytes <= sp->sfb_fc_threshold ||
		    bin->pkts == 0 || qsize == 0) {
			fcl = SFB_FC_LIST(sp, n);
		}
	} else if (bin->pkts <= (sp->sfb_allocation >> 2)) {
		fcl = SFB_FC_LIST(sp, n);
	}

	if (fcl != NULL && !STAILQ_EMPTY(&fcl->fclist)) {
		sfb_fclist_append(sp, fcl);
	}
	fcl = NULL;

	                                                        
	n = SFB_BINMASK(pkt_sfb_hash8[(s << 1) + 1]);
	bin = SFB_BINST(sp, 1, n, s);

	VERIFY(bin->pkts > 0 && bin->bytes >= (u_int64_t)pkt_len);
	bin->pkts--;
	bin->bytes -= pkt_len;
	if (bin->pkts == 0) {
		sfb_decrement_bin(sp, bin, SFB_BINFT(sp, 1, n, s), now);
	}
#else                                           
	for (i = 0; i < SFB_LEVELS; i++) {
		if (s == 0) {                                       
			n = SFB_BINMASK(pkt_sfb_hash8[i]);
		} else {                                            
			n = SFB_BINMASK(pkt_sfb_hash8[i + 2]);
		}

		bin = SFB_BINST(sp, i, n, s);

		VERIFY(bin->pkts > 0 && bin->bytes >= pkt_len);
		bin->pkts--;
		bin->bytes -= pkt_len;
		if (bin->pkts == 0) {
			sfb_decrement_bin(sp, bin,
			    SFB_BINFT(sp, i, n, s), now);
		}
		if (i != SFB_FC_LEVEL) {
			continue;
		}
		if (SFB_QUEUE_DELAYBASED(sp)) {
			if (!(SFB_IS_DELAYHIGH(sp)) ||
			    bin->bytes <= sp->sfb_fc_threshold) {
				fcl = SFB_FC_LIST(sp, n);
			}
		} else if (bin->pkts <= (sp->sfb_allocation >> 2)) {
			fcl = SFB_FC_LIST(sp, n);
		}
		if (fcl != NULL && !STAILQ_EMPTY(&fcl->fclist)) {
			sfb_fclist_append(sp, fcl);
		}
		fcl = NULL;
	}
#endif                                           
}

static inline void
sfb_eq_update_bins(struct sfb *sp, uint32_t pkt_sfb_hash, uint32_t pkt_len)
{
#if SFB_LEVELS != 2
	int i, n;
#endif                      
	int s;
	struct sfbbinstats *bin;
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;
	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	  
                                                  
    
#if SFB_LEVELS == 2
	                                                        
	bin = SFB_BINST(sp, 0,
	    SFB_BINMASK(pkt_sfb_hash8[(s << 1)]), s);
	bin->pkts++;
	bin->bytes += pkt_len;

	                                                        
	bin = SFB_BINST(sp, 1,
	    SFB_BINMASK(pkt_sfb_hash8[(s << 1) + 1]), s);
	bin->pkts++;
	bin->bytes += pkt_len;

#else                      
	for (i = 0; i < SFB_LEVELS; i++) {
		if (s == 0) {                                       
			n = SFB_BINMASK(pkt_sfb_hash8[i]);
		} else {                                            
			n = SFB_BINMASK(pkt_sfb_hash8[i + 2]);
		}

		bin = SFB_BINST(sp, i, n, s);
		bin->pkts++;
		bin->bytes += pkt_len;
	}
#endif                      
}

static boolean_t
sfb_bin_addfcentry(struct sfb *sp, pktsched_pkt_t *pkt, uint32_t pkt_sfb_hash,
    uint8_t flowsrc, uint32_t flowid)
{
	struct flowadv_fcentry *fce;
	struct sfb_fcl *fcl;
	int s;
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	if (flowid == 0) {
		sp->sfb_stats.null_flowid++;
		return FALSE;
	}

	  
                                      
                              
    
	fcl = SFB_FC_LIST(sp, SFB_BINMASK(pkt_sfb_hash8[(s << 1)]));
	STAILQ_FOREACH(fce, &fcl->fclist, fce_link) {
		if ((uint8_t)fce->fce_flowsrc_type == flowsrc &&
		    fce->fce_flowid == flowid) {
			                                               
			return TRUE;
		}
	}

	IFCQ_CONVERT_LOCK(&sp->sfb_ifp->if_snd);
	fce = pktsched_alloc_fcentry(pkt, sp->sfb_ifp, M_WAITOK);
	if (fce != NULL) {
		STAILQ_INSERT_TAIL(&fcl->fclist, fce, fce_link);
		fcl->cnt++;
		sp->sfb_stats.flow_controlled++;
	}

	return fce != NULL;
}

  
                                                            
                              
   
static int
sfb_bin_mark_or_drop(struct sfb *sp, struct sfbbinstats *bin)
{
	int ret = 0;
	if (SFB_QUEUE_DELAYBASED(sp)) {
		  
                                      
                                          
     
		if (SFB_IS_DELAYHIGH(sp) &&
		    bin->bytes >= (sp->sfb_fc_threshold << 1)) {
			ret = 1;
		}
	} else {
		if (bin->pkts >= sp->sfb_allocation &&
		    bin->pkts >= sp->sfb_drop_thresh) {
			ret = 1;                          
		}
	}
	return ret;
}

  
                                                                  
   
static int
sfb_drop_early(struct sfb *sp, uint32_t pkt_sfb_hash, u_int16_t *pmin,
    struct timespec *now)
{
#if SFB_LEVELS != 2
	int i;
#endif                      
	struct sfbbinstats *bin;
	int s, n, ret = 0;
	uint8_t *pkt_sfb_hash8 = (uint8_t *)&pkt_sfb_hash;

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	*pmin = (u_int16_t)-1;

	  
                                                  
    
#if SFB_LEVELS == 2
	                                                        
	n = SFB_BINMASK(pkt_sfb_hash8[(s << 1)]);
	bin = SFB_BINST(sp, 0, n, s);
	if (*pmin > (u_int16_t)bin->pmark) {
		*pmin = (u_int16_t)bin->pmark;
	}


	                            
	if (bin->pkts >= sp->sfb_allocation) {
		sfb_increment_bin(sp, bin, SFB_BINFT(sp, 0, n, s), now);
	}

	ret = sfb_bin_mark_or_drop(sp, bin);

	                                                        
	n = SFB_BINMASK(pkt_sfb_hash8[(s << 1) + 1]);
	bin = SFB_BINST(sp, 1, n, s);
	if (*pmin > (u_int16_t)bin->pmark) {
		*pmin = (u_int16_t)bin->pmark;
	}

	if (bin->pkts >= sp->sfb_allocation) {
		sfb_increment_bin(sp, bin, SFB_BINFT(sp, 1, n, s), now);
	}
#else                      
	for (i = 0; i < SFB_LEVELS; i++) {
		if (s == 0) {                                       
			n = SFB_BINMASK(pkt_sfb_hash8[i]);
		} else {                                            
			n = SFB_BINMASK(pkt_sfb_hash8[i + 2]);
		}

		bin = SFB_BINST(sp, i, n, s);
		if (*pmin > (u_int16_t)bin->pmark) {
			*pmin = (u_int16_t)bin->pmark;
		}

		if (bin->pkts >= sp->sfb_allocation) {
			sfb_increment_bin(sp, bin,
			    SFB_BINFT(sp, i, n, s), now);
		}
		if (i == SFB_FC_LEVEL) {
			ret = sfb_bin_mark_or_drop(sp, bin);
		}
	}
#endif                      

	if (sp->sfb_flags & SFBF_SUSPENDED) {
		ret = 1;                          
	}
	return ret;
}

void
sfb_detect_dequeue_stall(struct sfb *sp, class_queue_t *q,
    struct timespec *now)
{
	struct timespec max_getqtime;

	if (!SFB_QUEUE_DELAYBASED(sp) || SFB_IS_DELAYHIGH(sp) ||
	    qsize(q) <= SFB_MIN_FC_THRESHOLD_BYTES ||
	    !net_timerisset(&sp->sfb_getqtime)) {
		return;
	}

	net_timeradd(&sp->sfb_getqtime, &sp->sfb_update_interval,
	    &max_getqtime);
	if (net_timercmp(now, &max_getqtime, >)) {
		  
                                                        
                                                      
     
		SFB_SET_DELAY_HIGH(sp, q);
		sp->sfb_stats.dequeue_stall++;
	}
}

#define DTYPE_NODROP    0                    
#define DTYPE_FORCED    1                            
#define DTYPE_EARLY     2                                       

int
sfb_addq(struct sfb *sp, class_queue_t *q, pktsched_pkt_t *pkt,
    struct pf_mtag *t)
{
#if !PF_ECN
#pragma unused(t)
#endif              
	struct timespec now;
	int droptype, s;
	uint16_t pmin;
	int fc_adv = 0;
	int ret = CLASSQEQ_SUCCESS;
	uint32_t maxqsize = 0;
	uint64_t *pkt_timestamp;
	uint32_t *pkt_sfb_hash;
	uint16_t *pkt_sfb_hash16;
	uint32_t *pkt_sfb_flags;
	uint32_t pkt_flowid;
	volatile uint32_t *pkt_flags;
	uint8_t pkt_proto, pkt_flowsrc;

	s = sp->sfb_current;
	VERIFY((s + (s ^ 1)) == 1);

	pktsched_get_pkt_vars(pkt, &pkt_flags, &pkt_timestamp, &pkt_flowid,
	    &pkt_flowsrc, &pkt_proto, NULL);
	pkt_sfb_hash = pktsched_get_pkt_sfb_vars(pkt, &pkt_sfb_flags);
	pkt_sfb_hash16 = (uint16_t *)pkt_sfb_hash;

	switch (pkt->pktsched_ptype) {
	case QP_MBUF:
		                                               
		VERIFY(!(*pkt_flags & PKTF_PRIV_GUARDED));
		*pkt_flags |= PKTF_PRIV_GUARDED;
		break;
	default:
		VERIFY(0);
		                
		__builtin_unreachable();
	}

	if (*pkt_timestamp > 0) {
		net_nsectimer(pkt_timestamp, &now);
	} else {
		nanouptime(&now);
		net_timernsec(&now, pkt_timestamp);
	}

	                            
	if (net_timercmp(&now, &sp->sfb_nextreset, >=)) {
		net_timeradd(&now, &sp->sfb_hinterval, &sp->sfb_nextreset);
		sfb_swap_bins(sp, qlen(q));
		s = sp->sfb_current;
		VERIFY((s + (s ^ 1)) == 1);
	}

	if (!net_timerisset(&sp->sfb_update_time)) {
		net_timeradd(&now, &sp->sfb_update_interval,
		    &sp->sfb_update_time);
	}

	  
                                                            
                                                                 
    
	if (qsize(q) == 0 && !net_timerisset(&sp->sfb_getqtime)) {
		*(&sp->sfb_getqtime) = *(&now);
	}

	*pkt_sfb_flags = 0;
	pkt_sfb_hash16[s] =
	    (SFB_HASH(&pkt_flowid, sizeof(pkt_flowid),
	    (*sp->sfb_bins)[s].fudge) & SFB_HASHMASK);
	pkt_sfb_hash16[s ^ 1] =
	    (SFB_HASH(&pkt_flowid, sizeof(pkt_flowid),
	    (*sp->sfb_bins)[s ^ 1].fudge) & SFB_HASHMASK);

	                                         
	sfb_detect_dequeue_stall(sp, q, &now);

	                          
	droptype = DTYPE_NODROP;
	if (sfb_drop_early(sp, *pkt_sfb_hash, &pmin, &now)) {
		                                       
		if ((sp->sfb_flags & SFBF_FLOWCTL) &&
		    (*pkt_flags & PKTF_FLOW_ADV)) {
			fc_adv = 1;
			                                               
			if ((sp->sfb_flags & SFBF_SUSPENDED) ||
			    pkt_proto != IPPROTO_TCP) {
				droptype = DTYPE_EARLY;
				sp->sfb_stats.drop_early++;
			}
		}
#if PF_ECN
		                                  
		else if ((sp->sfb_flags & SFBF_ECN) &&
		    (pkt->pktsched_ptype == QP_MBUF) &&
		    (pkt_proto == IPPROTO_TCP) &&                   
		    ((sfb_random(sp) & SFB_MAX_PMARK) <= pmin) &&
		    mark_ecn(m, t, sp->sfb_flags) &&
		    !(sp->sfb_flags & SFBF_SUSPENDED)) {
			                                       
			sp->sfb_stats.marked_packets++;
		}
#endif             
		else {
			                          
			droptype = DTYPE_EARLY;
			sp->sfb_stats.drop_early++;
		}
	}

	                                  
	if (droptype == DTYPE_NODROP && sfb_penalize(sp, *pkt_sfb_hash,
	    pkt_sfb_flags, &now)) {
		droptype = DTYPE_FORCED;
		sp->sfb_stats.drop_pbox++;
	}

	if (SFB_QUEUE_DELAYBASED(sp)) {
		maxqsize = SFB_QUEUE_DELAYBASED_MAXSIZE;
	} else {
		maxqsize = qlimit(q);
	}

	  
                                                                
        
    
	if (droptype == DTYPE_NODROP && qlen(q) >= maxqsize) {
		if (pkt_proto == IPPROTO_TCP &&
		    qlen(q) < (maxqsize + (maxqsize >> 1)) &&
		    ((*pkt_flags & PKTF_TCP_REXMT) ||
		    (sp->sfb_flags & SFBF_LAST_PKT_DROPPED))) {
			  
                                                   
                                                     
                                                   
                                                
                                                
                                
      
			sp->sfb_flags &= ~SFBF_LAST_PKT_DROPPED;
		} else {
			droptype = DTYPE_FORCED;
			sp->sfb_stats.drop_queue++;
			sp->sfb_flags |= SFBF_LAST_PKT_DROPPED;
		}
	}

	if (fc_adv == 1 && droptype != DTYPE_FORCED &&
	    sfb_bin_addfcentry(sp, pkt, *pkt_sfb_hash, pkt_flowsrc,
	    pkt_flowid)) {
		                                         
		if (droptype == DTYPE_NODROP) {
			ret = CLASSQEQ_SUCCESS_FC;
			VERIFY(!(sp->sfb_flags & SFBF_SUSPENDED));
		} else if (sp->sfb_flags & SFBF_SUSPENDED) {
			                            
			ret = CLASSQEQ_DROP_SP;
		} else {
			                              
			ret = CLASSQEQ_DROP_FC;
		}
	}
	                                                     
	if (droptype == DTYPE_NODROP) {
		VERIFY(pkt->pktsched_ptype == qptype(q));
		_addq(q, &pkt->pktsched_pkt);
	} else {
		IFCQ_CONVERT_LOCK(&sp->sfb_ifp->if_snd);
		return (ret != CLASSQEQ_SUCCESS) ? ret : CLASSQEQ_DROP;
	}

	if (!(*pkt_sfb_flags & SFB_PKT_PBOX)) {
		sfb_eq_update_bins(sp, *pkt_sfb_hash,
		    pktsched_get_pkt_len(pkt));
	} else {
		sp->sfb_stats.pbox_packets++;
	}

	                         
	return ret;
}

static void *
sfb_getq_flow(struct sfb *sp, class_queue_t *q, u_int32_t flow, boolean_t purge,
    pktsched_pkt_t *pkt)
{
	struct timespec now;
	uint64_t *pkt_timestamp;
	volatile uint32_t *pkt_flags;
	uint32_t *pkt_sfb_flags;
	uint32_t *pkt_sfb_hash;
	classq_pkt_t p = CLASSQ_PKT_INITIALIZER(p);

	if (!purge && (sp->sfb_flags & SFBF_SUSPENDED)) {
		return NULL;
	}

	nanouptime(&now);

	                                   
	if (flow == 0) {
		_getq(q, &p);
	} else {
		_getq_flow(q, &p, flow);
	}

	if (p.cp_ptype == QP_INVALID) {
		if (!purge) {
			net_timerclear(&sp->sfb_getqtime);
		}
		return NULL;
	}

	pktsched_pkt_encap(pkt, &p);
	pktsched_get_pkt_vars(pkt, &pkt_flags, &pkt_timestamp, NULL,
	    NULL, NULL, NULL);
	pkt_sfb_hash = pktsched_get_pkt_sfb_vars(pkt, &pkt_sfb_flags);

	                                               
	switch (p.cp_ptype) {
	case QP_MBUF:
		VERIFY(*pkt_flags & PKTF_PRIV_GUARDED);
		break;
	default:
		VERIFY(0);
		                
		__builtin_unreachable();
	}

	if (!purge) {
		                                
		if (net_timerisset(&sp->sfb_getqtime)) {
			struct timespec delta;
			u_int64_t avg, new;
			net_timersub(&now, &sp->sfb_getqtime, &delta);
			net_timernsec(&delta, &new);
			avg = sp->sfb_stats.dequeue_avg;
			if (avg > 0) {
				int decay = DEQUEUE_DECAY;
				  
                                        
                                             
                                              
                     
       
				if (DEQUEUE_SPIKE(new, avg)) {
					decay += 5;
				}
				avg = (((avg << decay) - avg) + new) >> decay;
			} else {
				avg = new;
			}
			sp->sfb_stats.dequeue_avg = avg;
		}
		*(&sp->sfb_getqtime) = *(&now);
	}

	if (!purge && SFB_QUEUE_DELAYBASED(sp)) {
		u_int64_t dequeue_ns, queue_delay = 0;
		net_timernsec(&now, &dequeue_ns);
		if (dequeue_ns > *pkt_timestamp) {
			queue_delay = dequeue_ns - *pkt_timestamp;
		}

		if (sp->sfb_min_qdelay == 0 ||
		    (queue_delay > 0 && queue_delay < sp->sfb_min_qdelay)) {
			sp->sfb_min_qdelay = queue_delay;
		}
		if (net_timercmp(&now, &sp->sfb_update_time, >=)) {
			if (sp->sfb_min_qdelay > sp->sfb_target_qdelay) {
				if (!SFB_IS_DELAYHIGH(sp)) {
					SFB_SET_DELAY_HIGH(sp, q);
				}
			} else {
				sp->sfb_flags &= ~(SFBF_DELAYHIGH);
				sp->sfb_fc_threshold = 0;
			}
			net_timeradd(&now, &sp->sfb_update_interval,
			    &sp->sfb_update_time);
			sp->sfb_min_qdelay = 0;
		}
	}
	*pkt_timestamp = 0;

	  
                                                                
                                                                  
                                                                  
                                                                    
                                                                 
                                                                 
                                  
    
	if (*pkt_sfb_flags & SFB_PKT_PBOX) {
		*pkt_sfb_flags &= ~SFB_PKT_PBOX;
		if (sp->sfb_clearpkts > 0) {
			sp->sfb_clearpkts--;
		}
	} else if (sp->sfb_clearpkts > 0) {
		sp->sfb_clearpkts--;
	} else {
		sfb_dq_update_bins(sp, *pkt_sfb_hash, pktsched_get_pkt_len(pkt),
		    &now, qsize(q));
	}

	switch (p.cp_ptype) {
	case QP_MBUF:
		                                               
		*pkt_flags &= ~PKTF_PRIV_GUARDED;
		break;
	default:
		VERIFY(0);
		                
		__builtin_unreachable();
	}

	  
                                                                
                              
    
	if (qsize(q) == 0) {
		sp->sfb_flags &= ~SFBF_DELAYHIGH;
		sp->sfb_min_qdelay = 0;
		sp->sfb_fc_threshold = 0;
		net_timerclear(&sp->sfb_update_time);
		net_timerclear(&sp->sfb_getqtime);
	}
	return pkt->pktsched_pkt_mbuf;
}

void
sfb_getq(struct sfb *sp, class_queue_t *q, pktsched_pkt_t *pkt)
{
	sfb_getq_flow(sp, q, 0, FALSE, pkt);
}

void
sfb_purgeq(struct sfb *sp, class_queue_t *q, u_int32_t flow, u_int32_t *packets,
    u_int32_t *bytes)
{
	u_int32_t cnt = 0, len = 0;
	pktsched_pkt_t pkt;

	IFCQ_CONVERT_LOCK(&sp->sfb_ifp->if_snd);
	while (sfb_getq_flow(sp, q, flow, TRUE, &pkt) != NULL) {
		cnt++;
		len += pktsched_get_pkt_len(&pkt);
		pktsched_free_pkt(&pkt);
	}

	if (packets != NULL) {
		*packets = cnt;
	}
	if (bytes != NULL) {
		*bytes = len;
	}
}

void
sfb_updateq(struct sfb *sp, cqev_t ev)
{
	struct ifnet *ifp = sp->sfb_ifp;

	VERIFY(ifp != NULL);

	switch (ev) {
	case CLASSQ_EV_LINK_BANDWIDTH: {
		u_int64_t eff_rate = ifnet_output_linkrate(ifp);

		                                                
		if (eff_rate == sp->sfb_eff_rate) {
			break;
		}

		if (classq_verbose) {
			log(LOG_DEBUG, "%s: SFB qid=%d, adapting to new "
			    "eff_rate=%llu bps\n", if_name(ifp), sp->sfb_qid,
			    eff_rate);
		}
		sfb_calc_holdtime(sp, eff_rate);
		sfb_calc_pboxtime(sp, eff_rate);
		ifclassq_calc_target_qdelay(ifp, &sp->sfb_target_qdelay);
		sfb_calc_update_interval(sp, eff_rate);
		break;
	}

	case CLASSQ_EV_LINK_UP:
	case CLASSQ_EV_LINK_DOWN:
		if (classq_verbose) {
			log(LOG_DEBUG, "%s: SFB qid=%d, resetting due to "
			    "link %s\n", if_name(ifp), sp->sfb_qid,
			    (ev == CLASSQ_EV_LINK_UP) ? "UP" : "DOWN");
		}
		sfb_resetq(sp, ev);
		break;

	case CLASSQ_EV_LINK_LATENCY:
	case CLASSQ_EV_LINK_MTU:
	default:
		break;
	}
}

int
sfb_suspendq(struct sfb *sp, class_queue_t *q, boolean_t on)
{
#pragma unused(q)
	struct ifnet *ifp = sp->sfb_ifp;

	VERIFY(ifp != NULL);

	if ((on && (sp->sfb_flags & SFBF_SUSPENDED)) ||
	    (!on && !(sp->sfb_flags & SFBF_SUSPENDED))) {
		return 0;
	}

	if (!(sp->sfb_flags & SFBF_FLOWCTL)) {
		log(LOG_ERR, "%s: SFB qid=%d, unable to %s queue since "
		    "flow-control is not enabled", if_name(ifp), sp->sfb_qid,
		    (on ? "suspend" : "resume"));
		return ENOTSUP;
	}

	if (classq_verbose) {
		log(LOG_DEBUG, "%s: SFB qid=%d, setting state to %s",
		    if_name(ifp), sp->sfb_qid, (on ? "SUSPENDED" : "RUNNING"));
	}

	if (on) {
		sp->sfb_flags |= SFBF_SUSPENDED;
	} else {
		sp->sfb_flags &= ~SFBF_SUSPENDED;
		sfb_swap_bins(sp, qlen(q));
	}

	return 0;
}
