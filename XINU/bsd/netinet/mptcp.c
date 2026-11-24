/*
 * Copyright (c) 2012-2018 Apple Inc. All rights reserved.
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

  
                                         
  
                                                                        
                                                                          
                                                       
                                                                       
                                                                           
                                            
  
                                                                           
                                                   
  
                                     
  
                                                                             
                                                                                
                                                                            
                                                                              
                                                                   
                                                                            
                                             
  
                                                                               
                                                                            
                                                                              
  
                                                                      
                                                                             
                                                                           
                                                                           
                                                                      
                                                                          
                                                                           
                                                                           
                                                                             
                                                                                
  
                                                                             
                                                                              
                                                                            
                                                                     
                                                                      
                                                                             
                                                                            
                                                                             
                                                                             
                                                                               
                                                                           
                                                                              
                     
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>
#include <sys/protosw.h>

#include <kern/zalloc.h>
#include <kern/locks.h>

#include <mach/sdt.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_var.h>
#include <netinet/mptcp_var.h>
#include <netinet/mptcp.h>
#include <netinet/mptcp_seq.h>
#include <netinet/mptcp_opt.h>
#include <netinet/mptcp_timer.h>

int mptcp_enable = 1;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, enable, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_enable, 0, "Enable Multipath TCP Support");

  
                                                                   
                                                                               
                                                                                
   
int mptcp_mpcap_retries = 4;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, mptcp_cap_retr,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_mpcap_retries, 0, "Number of MP Capable SYN Retries");

  
                                                                
                             
   
int mptcp_dss_csum = 0;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, dss_csum, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_dss_csum, 0, "Enable DSS checksum");

  
                                                                              
                                    
   
int mptcp_fail_thresh = 1;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, fail, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_fail_thresh, 0, "Failover threshold");

  
                                                                            
                                                                        
                                                            
   
int mptcp_subflow_keeptime = 60 * 14;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, keepalive, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_subflow_keeptime, 0, "Keepalive in seconds");

int mptcp_rtthist_rtthresh = 600;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, rtthist_thresh, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_rtthist_rtthresh, 0, "Rtt threshold");

int mptcp_rtothresh = 1500;
SYSCTL_INT(_net_inet_mptcp, OID_AUTO, rto_thresh, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_rtothresh, 0, "RTO threshold");

  
                                                  
   
uint32_t mptcp_probeto = 1000;
SYSCTL_UINT(_net_inet_mptcp, OID_AUTO, probeto, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_probeto, 0, "Disable probing by setting to 0");

uint32_t mptcp_probecnt = 5;
SYSCTL_UINT(_net_inet_mptcp, OID_AUTO, probecnt, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mptcp_probecnt, 0, "Number of probe writes");

static int
mptcp_reass_present(struct socket *mp_so)
{
	struct mptses *mpte = mpsotompte(mp_so);
	struct mptcb *mp_tp = mpte->mpte_mptcb;
	struct tseg_qent *q;
	int dowakeup = 0;
	int flags = 0;

	  
                                                   
                             
    
	if (mp_tp->mpt_state < MPTCPS_ESTABLISHED) {
		return flags;
	}
	q = LIST_FIRST(&mp_tp->mpt_segq);
	if (!q || q->tqe_m->m_pkthdr.mp_dsn != mp_tp->mpt_rcvnxt) {
		return flags;
	}

	  
                                                                
                                                        
                    
    
	if (mp_tp->mpt_flags & MPTCPF_REASS_INPROG) {
		return flags;
	}

	mp_tp->mpt_flags |= MPTCPF_REASS_INPROG;

	do {
		mp_tp->mpt_rcvnxt += q->tqe_len;
		LIST_REMOVE(q, tqe_q);
		if (mp_so->so_state & SS_CANTRCVMORE) {
			m_freem(q->tqe_m);
		} else {
			flags = !!(q->tqe_m->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN);
			if (sbappendstream_rcvdemux(mp_so, q->tqe_m)) {
				dowakeup = 1;
			}
		}
		zfree(tcp_reass_zone, q);
		mp_tp->mpt_reassqlen--;
		q = LIST_FIRST(&mp_tp->mpt_segq);
	} while (q && q->tqe_m->m_pkthdr.mp_dsn == mp_tp->mpt_rcvnxt);
	mp_tp->mpt_flags &= ~MPTCPF_REASS_INPROG;

	if (dowakeup) {
		sorwakeup(mp_so);                                 
	}
	return flags;
}

static int
mptcp_reass(struct socket *mp_so, struct pkthdr *phdr, int *tlenp, struct mbuf *m)
{
	struct mptcb *mp_tp = mpsotomppcb(mp_so)->mpp_pcbe->mpte_mptcb;
	u_int64_t mb_dsn = phdr->mp_dsn;
	struct tseg_qent *q;
	struct tseg_qent *p = NULL;
	struct tseg_qent *nq;
	struct tseg_qent *te = NULL;
	uint32_t qlimit;

	  
                                                                   
                                                                    
                                                                  
                                                                  
                                
    
	qlimit = MIN(MAX(100, mp_so->so_rcv.sb_hiwat >> 10),
	    (tcp_autorcvbuf_max >> 10));
	if (mb_dsn != mp_tp->mpt_rcvnxt &&
	    (mp_tp->mpt_reassqlen + 1) >= qlimit) {
		tcpstat.tcps_mptcp_rcvmemdrop++;
		m_freem(m);
		*tlenp = 0;
		return 0;
	}

	                                                                     
	te = (struct tseg_qent *) zalloc(tcp_reass_zone);
	if (te == NULL) {
		tcpstat.tcps_mptcp_rcvmemdrop++;
		m_freem(m);
		return 0;
	}

	mp_tp->mpt_reassqlen++;

	  
                                                    
    
	LIST_FOREACH(q, &mp_tp->mpt_segq, tqe_q) {
		if (MPTCP_SEQ_GT(q->tqe_m->m_pkthdr.mp_dsn, mb_dsn)) {
			break;
		}
		p = q;
	}

	  
                                                           
                                                             
                                                      
    
	if (p != NULL) {
		int64_t i;
		                                                     
		i = p->tqe_m->m_pkthdr.mp_dsn + p->tqe_len - mb_dsn;
		if (i > 0) {
			if (i >= *tlenp) {
				tcpstat.tcps_mptcp_rcvduppack++;
				m_freem(m);
				zfree(tcp_reass_zone, te);
				te = NULL;
				mp_tp->mpt_reassqlen--;
				  
                                     
                                           
                                     
                 
       
				goto out;
			}
			VERIFY(i <= INT_MAX);
			m_adj(m, (int)i);
			*tlenp -= i;
			phdr->mp_dsn += i;
		}
	}

	tcpstat.tcps_mp_oodata++;

	  
                                                      
                                                 
    
	while (q) {
		int64_t i = (mb_dsn + *tlenp) - q->tqe_m->m_pkthdr.mp_dsn;
		if (i <= 0) {
			break;
		}

		if (i < q->tqe_len) {
			q->tqe_m->m_pkthdr.mp_dsn += i;
			q->tqe_len -= i;

			VERIFY(i <= INT_MAX);
			m_adj(q->tqe_m, (int)i);
			break;
		}

		nq = LIST_NEXT(q, tqe_q);
		LIST_REMOVE(q, tqe_q);
		m_freem(q->tqe_m);
		zfree(tcp_reass_zone, q);
		mp_tp->mpt_reassqlen--;
		q = nq;
	}

	                                                    
	te->tqe_m = m;
	te->tqe_th = NULL;
	te->tqe_len = *tlenp;

	if (p == NULL) {
		LIST_INSERT_HEAD(&mp_tp->mpt_segq, te, tqe_q);
	} else {
		LIST_INSERT_AFTER(p, te, tqe_q);
	}

out:
	return mptcp_reass_present(mp_so);
}

  
                                                                     
   
void
mptcp_input(struct mptses *mpte, struct mbuf *m)
{
	struct socket *mp_so;
	struct mptcb *mp_tp = NULL;
	int count = 0, wakeup = 0;
	struct mbuf *save = NULL, *prev = NULL;
	struct mbuf *freelist = NULL, *tail = NULL;

	VERIFY(m->m_flags & M_PKTHDR);

	mp_so = mptetoso(mpte);
	mp_tp = mpte->mpte_mptcb;

	socket_lock_assert_owned(mp_so);

	DTRACE_MPTCP(input);

	mp_tp->mpt_rcvwnd = mptcp_sbspace(mp_tp);

	  
                                              
                                                             
                
   
    
	count = mp_so->so_rcv.sb_cc;

	  
                                                                   
    
	if (mp_tp->mpt_flags & MPTCPF_FALLBACK_TO_TCP) {
		struct mbuf *iter;
		int mb_dfin = 0;
fallback:
		mptcp_sbrcv_grow(mp_tp);

		iter = m;
		while (iter) {
			if ((iter->m_flags & M_PKTHDR) &&
			    (iter->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN)) {
				mb_dfin = 1;
			}

			if ((iter->m_flags & M_PKTHDR) && m_pktlen(iter) == 0) {
				                                                
				if (prev == NULL) {
					m = iter->m_next;
					m_free(iter);
					iter = m;
				} else {
					prev->m_next = iter->m_next;
					m_free(iter);
					iter = prev->m_next;
				}

				                                                              
				VERIFY(iter == NULL || iter->m_flags & M_PKTHDR);
			} else {
				prev = iter;
				iter = iter->m_next;
			}
		}

		  
                                                         
                                                           
     
		if (sbappendstream_rcvdemux(mp_so, m)) {
			sorwakeup(mp_so);
		}

		DTRACE_MPTCP5(receive__degraded, struct mbuf *, m,
		    struct socket *, mp_so,
		    struct sockbuf *, &mp_so->so_rcv,
		    struct sockbuf *, &mp_so->so_snd,
		    struct mptses *, mpte);
		count = mp_so->so_rcv.sb_cc - count;

		mp_tp->mpt_rcvnxt += count;

		if (mb_dfin) {
			mptcp_close_fsm(mp_tp, MPCE_RECV_DATA_FIN);
			socantrcvmore(mp_so);
		}
		return;
	}

	do {
		u_int64_t mb_dsn;
		int32_t mb_datalen;
		int64_t todrop;
		int mb_dfin = 0;

		VERIFY(m->m_flags & M_PKTHDR);

		                                                            
		if (!(m->m_pkthdr.pkt_flags & PKTF_MPTCP)) {
			goto fallback;
		}

		save = m->m_next;
		  
                                                 
                                                      
                                                   
                                                   
                                                    
                                                    
                                                 
     
		while (save && (!(save->m_flags & M_PKTHDR) ||
		    !(save->m_pkthdr.pkt_flags & PKTF_MPTCP))) {
			prev = save;
			save = save->m_next;
		}
		if (prev) {
			prev->m_next = NULL;
		} else {
			m->m_next = NULL;
		}

		mb_dsn = m->m_pkthdr.mp_dsn;
		mb_datalen = m->m_pkthdr.mp_rlen;

		todrop = (mb_dsn + mb_datalen) - (mp_tp->mpt_rcvnxt + mp_tp->mpt_rcvwnd);
		if (todrop > 0) {
			tcpstat.tcps_mptcp_rcvpackafterwin++;

			os_log_info(mptcp_log_handle, "%s - %lx: dropping dsn %u dlen %u rcvnxt %u rcvwnd %u todrop %lld\n",
			    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mpte),
			    (uint32_t)mb_dsn, mb_datalen, (uint32_t)mp_tp->mpt_rcvnxt,
			    mp_tp->mpt_rcvwnd, todrop);

			if (todrop >= mb_datalen) {
				if (freelist == NULL) {
					freelist = m;
				} else {
					tail->m_next = m;
				}

				if (prev != NULL) {
					tail = prev;
				} else {
					tail = m;
				}

				m = save;
				prev = save = NULL;
				continue;
			} else {
				VERIFY(todrop <= INT_MAX);
				m_adj(m, (int)-todrop);
				mb_datalen -= todrop;
				m->m_pkthdr.mp_rlen -= todrop;
			}

			  
                                                       
                                 
      
			m->m_pkthdr.pkt_flags &= ~PKTF_MPTCP_DFIN;
		}

		if (MPTCP_SEQ_LT(mb_dsn, mp_tp->mpt_rcvnxt)) {
			if (MPTCP_SEQ_LEQ((mb_dsn + mb_datalen),
			    mp_tp->mpt_rcvnxt)) {
				if (freelist == NULL) {
					freelist = m;
				} else {
					tail->m_next = m;
				}

				if (prev != NULL) {
					tail = prev;
				} else {
					tail = m;
				}

				m = save;
				prev = save = NULL;
				continue;
			} else {
				VERIFY((mp_tp->mpt_rcvnxt - mb_dsn) <= INT_MAX);
				m_adj(m, (int)(mp_tp->mpt_rcvnxt - mb_dsn));
				mb_datalen -= (mp_tp->mpt_rcvnxt - mb_dsn);
				mb_dsn = mp_tp->mpt_rcvnxt;
				VERIFY(mb_datalen >= 0 && mb_datalen <= USHRT_MAX);
				m->m_pkthdr.mp_rlen = (uint16_t)mb_datalen;
				m->m_pkthdr.mp_dsn = mb_dsn;
			}
		}

		if (MPTCP_SEQ_GT(mb_dsn, mp_tp->mpt_rcvnxt) ||
		    !LIST_EMPTY(&mp_tp->mpt_segq)) {
			mb_dfin = mptcp_reass(mp_so, &m->m_pkthdr, &mb_datalen, m);

			goto next;
		}
		mb_dfin = !!(m->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN);

		mptcp_sbrcv_grow(mp_tp);

		if (sbappendstream_rcvdemux(mp_so, m)) {
			wakeup = 1;
		}

		DTRACE_MPTCP6(receive, struct mbuf *, m, struct socket *, mp_so,
		    struct sockbuf *, &mp_so->so_rcv,
		    struct sockbuf *, &mp_so->so_snd,
		    struct mptses *, mpte,
		    struct mptcb *, mp_tp);
		count = mp_so->so_rcv.sb_cc - count;
		tcpstat.tcps_mp_rcvtotal++;
		tcpstat.tcps_mp_rcvbytes += count;

		mp_tp->mpt_rcvnxt += count;

next:
		if (mb_dfin) {
			mptcp_close_fsm(mp_tp, MPCE_RECV_DATA_FIN);
			socantrcvmore(mp_so);
		}
		m = save;
		prev = save = NULL;
		count = mp_so->so_rcv.sb_cc;
	} while (m);

	if (freelist) {
		m_freem(freelist);
	}

	if (wakeup) {
		sorwakeup(mp_so);
	}
}

boolean_t
mptcp_can_send_more(struct mptcb *mp_tp, boolean_t ignore_reinject)
{
	struct socket *mp_so = mptetoso(mp_tp->mpt_mpte);

	  
                                                       
    
	if (!ignore_reinject && mp_tp->mpt_mpte->mpte_reinjectq) {
		return TRUE;
	}

	  
                   
   
                                                                      
                                                               
                                                                      
                                                                          
    

	if (!(mp_so->so_flags1 & SOF1_PRECONNECT_DATA) && MPTCP_SEQ_GEQ(mp_tp->mpt_sndnxt, mp_tp->mpt_sndmax)) {
		return FALSE;
	}

	if (MPTCP_SEQ_LEQ(mp_tp->mpt_snduna + mp_tp->mpt_sndwnd, mp_tp->mpt_sndnxt)) {
		return FALSE;
	}

	if (mp_tp->mpt_sndnxt + 1 == mp_tp->mpt_sndmax && mp_tp->mpt_state > MPTCPS_CLOSE_WAIT) {
		return FALSE;
	}

	if (mp_tp->mpt_state >= MPTCPS_FIN_WAIT_2) {
		return FALSE;
	}

	return TRUE;
}

  
                
   
int
mptcp_output(struct mptses *mpte)
{
	struct mptcb *mp_tp;
	struct mptsub *mpts;
	struct mptsub *mpts_tried = NULL;
	struct socket *mp_so;
	struct mptsub *preferred_mpts = NULL;
	uint64_t old_snd_nxt;
	int error = 0;

	mp_so = mptetoso(mpte);
	mp_tp = mpte->mpte_mptcb;

	socket_lock_assert_owned(mp_so);

	if (mp_so->so_flags & SOF_DEFUNCT) {
		return 0;
	}

	VERIFY(!(mpte->mpte_mppcb->mpp_flags & MPP_WUPCALL));
	mpte->mpte_mppcb->mpp_flags |= MPP_WUPCALL;

	old_snd_nxt = mp_tp->mpt_sndnxt;
	while (mptcp_can_send_more(mp_tp, FALSE)) {
		                                                        
		mpts = mptcp_get_subflow(mpte, &preferred_mpts);
		if (mpts == NULL) {
			mptcplog((LOG_INFO, "%s: no subflow\n", __func__),
			    MPTCP_SENDER_DBG, MPTCP_LOGLVL_LOG);
			break;
		}

		                                                       
		if (mpts_tried != NULL &&
		    (mpts == mpts_tried || (mpts->mpts_flags & MPTSF_FAILINGOVER))) {
			mpts_tried->mpts_flags &= ~MPTSF_FAILINGOVER;
			mpts_tried->mpts_flags |= MPTSF_ACTIVE;
			mptcp_start_timer(mpte, MPTT_REXMT);
			break;
		}

		  
                                                              
                                                                
                                                          
                                                                
                                     
     
		if ((mp_so->so_snd.sb_flags & (SB_AUTOSIZE | SB_TRIM)) == SB_AUTOSIZE &&
		    tcp_cansbgrow(&mp_so->so_snd)) {
			if ((mp_tp->mpt_sndwnd / 4 * 5) >= mp_so->so_snd.sb_hiwat &&
			    mp_so->so_snd.sb_cc >= (mp_so->so_snd.sb_hiwat / 8 * 7)) {
				if (sbreserve(&mp_so->so_snd,
				    min(mp_so->so_snd.sb_hiwat + tcp_autosndbuf_inc,
				    tcp_autosndbuf_max)) == 1) {
					mp_so->so_snd.sb_idealsize = mp_so->so_snd.sb_hiwat;
				}
			}
		}

		DTRACE_MPTCP3(output, struct mptses *, mpte, struct mptsub *, mpts,
		    struct socket *, mp_so);
		error = mptcp_subflow_output(mpte, mpts, 0);
		if (error) {
			                                                              
			mpts->mpts_flags |= MPTSF_FAILINGOVER;
			mpts->mpts_flags &= ~MPTSF_ACTIVE;
			mpts_tried = mpts;
			if (error != ECANCELED) {
				os_log_error(mptcp_log_handle, "%s - %lx: Error = %d mpts_flags %#x\n",
				    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mpte),
				    error, mpts->mpts_flags);
			}
			break;
		}
		                                                         
		mpts->mpts_flags |= MPTSF_ACTIVE;
		mpts->mpts_probesoon = mpts->mpts_probecnt = 0;

		                                          
		if (mptcp_probeto && mpts != preferred_mpts && preferred_mpts != NULL) {
			if (preferred_mpts->mpts_probesoon) {
				if ((tcp_now - preferred_mpts->mpts_probesoon) > mptcp_probeto) {
					mptcp_subflow_output(mpte, preferred_mpts, MPTCP_SUBOUT_PROBING);
					if (preferred_mpts->mpts_probecnt >= mptcp_probecnt) {
						preferred_mpts->mpts_probesoon = 0;
						preferred_mpts->mpts_probecnt = 0;
					}
				}
			} else {
				preferred_mpts->mpts_probesoon = tcp_now;
				preferred_mpts->mpts_probecnt = 0;
			}
		}

		if (mpte->mpte_active_sub == NULL) {
			mpte->mpte_active_sub = mpts;
		} else if (mpte->mpte_active_sub != mpts) {
			mpte->mpte_active_sub->mpts_flags &= ~MPTSF_ACTIVE;
			mpte->mpte_active_sub = mpts;

			mptcpstats_inc_switch(mpte, mpts);
		}
	}

	if (mp_tp->mpt_state > MPTCPS_CLOSE_WAIT) {
		if (mp_tp->mpt_sndnxt + 1 == mp_tp->mpt_sndmax &&
		    mp_tp->mpt_snduna == mp_tp->mpt_sndnxt) {
			mptcp_finish_usrclosed(mpte);
		}
	}

	mptcp_handle_deferred_upcalls(mpte->mpte_mppcb, MPP_WUPCALL);

	                                                     
	return 0;
}


static struct mptsub *
mptcp_choose_subflow(struct mptsub *mpts, struct mptsub *curbest, int *currtt)
{
	struct tcpcb *tp = sototcpcb(mpts->mpts_socket);

	  
                                                 
                                                   
                 
    
	if (tp->t_srtt && *currtt > tp->t_srtt &&
	    (curbest == NULL || tp->t_rxtshift == 0 ||
	    sototcpcb(curbest->mpts_socket)->t_rxtshift)) {
		*currtt = tp->t_srtt;
		return mpts;
	}

	  
                                                      
    
	if (curbest &&
	    sototcpcb(curbest->mpts_socket)->t_rxtshift &&
	    tp->t_rxtshift == 0) {
		*currtt = tp->t_srtt;
		return mpts;
	}

	return curbest != NULL ? curbest : mpts;
}

static struct mptsub *
mptcp_return_subflow(struct mptsub *mpts)
{
	if (mpts && mptcp_subflow_cwnd_space(mpts->mpts_socket) <= 0) {
		return NULL;
	}

	return mpts;
}

static boolean_t
mptcp_subflow_is_slow(struct mptses *mpte, struct mptsub *mpts)
{
	struct tcpcb *tp = sototcpcb(mpts->mpts_socket);
	int fail_thresh = mptcp_fail_thresh;

	if (mpte->mpte_svctype == MPTCP_SVCTYPE_HANDOVER || mpte->mpte_svctype == MPTCP_SVCTYPE_PURE_HANDOVER) {
		fail_thresh *= 2;
	}

	return tp->t_rxtshift >= fail_thresh &&
	       (mptetoso(mpte)->so_snd.sb_cc || mpte->mpte_reinjectq);
}

  
                                                                
   
struct mptsub *
mptcp_get_subflow(struct mptses *mpte, struct mptsub **preferred)
{
	struct tcpcb *besttp, *secondtp;
	struct inpcb *bestinp, *secondinp;
	struct mptsub *mpts;
	struct mptsub *best = NULL;
	struct mptsub *second_best = NULL;
	int exp_rtt = INT_MAX, cheap_rtt = INT_MAX;

	  
               
                                                                     
    

	TAILQ_FOREACH(mpts, &mpte->mpte_subflows, mpts_entry) {
		struct socket *so = mpts->mpts_socket;
		struct tcpcb *tp = sototcpcb(so);
		struct inpcb *inp = sotoinpcb(so);

		mptcplog((LOG_DEBUG, "%s mpts %u mpts_flags %#x, suspended %u sostate %#x tpstate %u cellular %d rtt %u rxtshift %u cheap %u exp %u cwnd %d\n",
		    __func__, mpts->mpts_connid, mpts->mpts_flags,
		    INP_WAIT_FOR_IF_FEEDBACK(inp), so->so_state, tp->t_state,
		    inp->inp_last_outifp ? IFNET_IS_CELLULAR(inp->inp_last_outifp) : -1,
		    tp->t_srtt, tp->t_rxtshift, cheap_rtt, exp_rtt,
		    mptcp_subflow_cwnd_space(so)),
		    MPTCP_SOCKET_DBG, MPTCP_LOGLVL_VERBOSE);

		  
                                                  
                              
     
		if (inp->inp_last_outifp == NULL) {
			continue;
		}

		if (INP_WAIT_FOR_IF_FEEDBACK(inp)) {
			continue;
		}

		                                                     
		if (mpts->mpts_flags & MPTSF_MP_DEGRADED) {
			best = mpts;
			break;
		}

		  
                                                       
     
		if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
			return mptcp_return_subflow(mpts);
		}

		  
                                                              
                                                               
     
		if (!(mpts->mpts_flags & MPTSF_MP_CAPABLE)) {
			continue;
		}

		if ((so->so_state & SS_ISDISCONNECTED) ||
		    !(so->so_state & SS_ISCONNECTED) ||
		    !TCPS_HAVEESTABLISHED(tp->t_state) ||
		    tp->t_state > TCPS_CLOSE_WAIT) {
			continue;
		}

		  
                                                              
                                                           
     
		if (IFNET_IS_CELLULAR(inp->inp_last_outifp)) {
			second_best = mptcp_choose_subflow(mpts, second_best,
			    &exp_rtt);
		} else {
			best = mptcp_choose_subflow(mpts, best, &cheap_rtt);
		}
	}

	  
                                                                      
                                        
    
	if (best == NULL) {
		return mptcp_return_subflow(second_best);
	}

	if (second_best == NULL) {
		return mptcp_return_subflow(best);
	}

	besttp = sototcpcb(best->mpts_socket);
	bestinp = sotoinpcb(best->mpts_socket);
	secondtp = sototcpcb(second_best->mpts_socket);
	secondinp = sotoinpcb(second_best->mpts_socket);

	if (preferred != NULL) {
		*preferred = mptcp_return_subflow(best);
	}

	  
                                                                   
                                                      
    
	if (mpte->mpte_svctype == MPTCP_SVCTYPE_PURE_HANDOVER) {
		return mptcp_return_subflow(best);
	} else if (mpte->mpte_svctype == MPTCP_SVCTYPE_HANDOVER) {
		  
                                                 
     
		if (!IFNET_IS_CELLULAR(bestinp->inp_last_outifp) &&
		    mptcp_is_wifi_unusable_for_session(mpte) != 0 && mptcp_subflow_is_slow(mpte, best)) {
			return mptcp_return_subflow(second_best);
		}

		return mptcp_return_subflow(best);
	} else if (mpte->mpte_svctype == MPTCP_SVCTYPE_INTERACTIVE) {
		int rtt_thresh = mptcp_rtthist_rtthresh << TCP_RTT_SHIFT;
		int rto_thresh = mptcp_rtothresh;

		                                      
		if (!IFNET_IS_CELLULAR(bestinp->inp_last_outifp) &&
		    mptcp_is_wifi_unusable_for_session(mpte) != 0) {
			rtt_thresh /= 2;
			rto_thresh /= 2;
		}

		if (besttp->t_srtt && secondtp->t_srtt &&
		    besttp->t_srtt >= rtt_thresh &&
		    secondtp->t_srtt < rtt_thresh) {
			tcpstat.tcps_mp_sel_rtt++;
			mptcplog((LOG_DEBUG, "%s: best cid %d at rtt %d,  second cid %d at rtt %d\n", __func__,
			    best->mpts_connid, besttp->t_srtt >> TCP_RTT_SHIFT,
			    second_best->mpts_connid,
			    secondtp->t_srtt >> TCP_RTT_SHIFT),
			    MPTCP_SENDER_DBG, MPTCP_LOGLVL_LOG);
			return mptcp_return_subflow(second_best);
		}

		if (mptcp_subflow_is_slow(mpte, best) &&
		    secondtp->t_rxtshift == 0) {
			return mptcp_return_subflow(second_best);
		}

		                                                                      
		if (besttp->t_rxtcur && secondtp->t_rxtcur &&
		    besttp->t_rxtcur >= rto_thresh &&
		    secondtp->t_rxtcur < rto_thresh) {
			tcpstat.tcps_mp_sel_rto++;
			mptcplog((LOG_DEBUG, "%s: best cid %d at rto %d, second cid %d at rto %d\n", __func__,
			    best->mpts_connid, besttp->t_rxtcur,
			    second_best->mpts_connid, secondtp->t_rxtcur),
			    MPTCP_SENDER_DBG, MPTCP_LOGLVL_LOG);

			return mptcp_return_subflow(second_best);
		}

		  
                                                              
                                                               
                                             
     
		return mptcp_return_subflow(best);
	} else if (mpte->mpte_svctype >= MPTCP_SVCTYPE_AGGREGATE) {
		struct mptsub *tmp;

		  
                                            
     
		if (besttp->t_srtt > secondtp->t_srtt) {
			tmp = best;
			best = second_best;
			besttp = secondtp;
			bestinp = secondinp;

			second_best = tmp;
			secondtp = sototcpcb(second_best->mpts_socket);
			secondinp = sotoinpcb(second_best->mpts_socket);
		}

		                                                    
		if (mptcp_subflow_cwnd_space(bestinp->inp_socket) <= 0) {
			return mptcp_return_subflow(second_best);
		}

		return mptcp_return_subflow(best);
	} else {
		panic("Unknown service-type configured for MPTCP");
	}

	return NULL;
}

static const char *
mptcp_event_to_str(uint32_t event)
{
	const char *c = "UNDEFINED";
	switch (event) {
	case MPCE_CLOSE:
		c = "MPCE_CLOSE";
		break;
	case MPCE_RECV_DATA_ACK:
		c = "MPCE_RECV_DATA_ACK";
		break;
	case MPCE_RECV_DATA_FIN:
		c = "MPCE_RECV_DATA_FIN";
		break;
	}
	return c;
}

static const char *
mptcp_state_to_str(mptcp_state_t state)
{
	const char *c = "UNDEFINED";
	switch (state) {
	case MPTCPS_CLOSED:
		c = "MPTCPS_CLOSED";
		break;
	case MPTCPS_LISTEN:
		c = "MPTCPS_LISTEN";
		break;
	case MPTCPS_ESTABLISHED:
		c = "MPTCPS_ESTABLISHED";
		break;
	case MPTCPS_CLOSE_WAIT:
		c = "MPTCPS_CLOSE_WAIT";
		break;
	case MPTCPS_FIN_WAIT_1:
		c = "MPTCPS_FIN_WAIT_1";
		break;
	case MPTCPS_CLOSING:
		c = "MPTCPS_CLOSING";
		break;
	case MPTCPS_LAST_ACK:
		c = "MPTCPS_LAST_ACK";
		break;
	case MPTCPS_FIN_WAIT_2:
		c = "MPTCPS_FIN_WAIT_2";
		break;
	case MPTCPS_TIME_WAIT:
		c = "MPTCPS_TIME_WAIT";
		break;
	case MPTCPS_TERMINATE:
		c = "MPTCPS_TERMINATE";
		break;
	}
	return c;
}

void
mptcp_close_fsm(struct mptcb *mp_tp, uint32_t event)
{
	struct socket *mp_so = mptetoso(mp_tp->mpt_mpte);

	socket_lock_assert_owned(mp_so);

	mptcp_state_t old_state = mp_tp->mpt_state;

	DTRACE_MPTCP2(state__change, struct mptcb *, mp_tp,
	    uint32_t, event);

	switch (mp_tp->mpt_state) {
	case MPTCPS_CLOSED:
	case MPTCPS_LISTEN:
		mp_tp->mpt_state = MPTCPS_TERMINATE;
		break;

	case MPTCPS_ESTABLISHED:
		if (event == MPCE_CLOSE) {
			mp_tp->mpt_state = MPTCPS_FIN_WAIT_1;
			mp_tp->mpt_sndmax += 1;                          
		} else if (event == MPCE_RECV_DATA_FIN) {
			mp_tp->mpt_rcvnxt += 1;                          
			mp_tp->mpt_state = MPTCPS_CLOSE_WAIT;
		}
		break;

	case MPTCPS_CLOSE_WAIT:
		if (event == MPCE_CLOSE) {
			mp_tp->mpt_state = MPTCPS_LAST_ACK;
			mp_tp->mpt_sndmax += 1;                          
		}
		break;

	case MPTCPS_FIN_WAIT_1:
		if (event == MPCE_RECV_DATA_ACK) {
			mp_tp->mpt_state = MPTCPS_FIN_WAIT_2;
		} else if (event == MPCE_RECV_DATA_FIN) {
			mp_tp->mpt_rcvnxt += 1;                          
			mp_tp->mpt_state = MPTCPS_CLOSING;
		}
		break;

	case MPTCPS_CLOSING:
		if (event == MPCE_RECV_DATA_ACK) {
			mp_tp->mpt_state = MPTCPS_TIME_WAIT;
		}
		break;

	case MPTCPS_LAST_ACK:
		if (event == MPCE_RECV_DATA_ACK) {
			mptcp_close(mp_tp->mpt_mpte, mp_tp);
		}
		break;

	case MPTCPS_FIN_WAIT_2:
		if (event == MPCE_RECV_DATA_FIN) {
			mp_tp->mpt_rcvnxt += 1;                          
			mp_tp->mpt_state = MPTCPS_TIME_WAIT;
		}
		break;

	case MPTCPS_TIME_WAIT:
	case MPTCPS_TERMINATE:
		break;

	default:
		VERIFY(0);
		                
	}
	DTRACE_MPTCP2(state__change, struct mptcb *, mp_tp,
	    uint32_t, event);
	mptcplog((LOG_INFO, "%s: %s to %s on event %s\n", __func__,
	    mptcp_state_to_str(old_state),
	    mptcp_state_to_str(mp_tp->mpt_state),
	    mptcp_event_to_str(event)),
	    MPTCP_STATE_DBG, MPTCP_LOGLVL_LOG);
}

                                                                    
void
mptcp_update_dss_rcv_state(struct mptcp_dsn_opt *dss_info, struct tcpcb *tp,
    uint16_t csum)
{
	struct mptcb *mp_tp = tptomptp(tp);
	u_int64_t full_dsn = 0;

	NTOHL(dss_info->mdss_dsn);
	NTOHL(dss_info->mdss_subflow_seqn);
	NTOHS(dss_info->mdss_data_len);

	                                     
	MPTCP_EXTEND_DSN(mp_tp->mpt_rcvnxt, dss_info->mdss_dsn, full_dsn);
	mptcp_update_rcv_state_meat(mp_tp, tp,
	    full_dsn, dss_info->mdss_subflow_seqn, dss_info->mdss_data_len,
	    csum);
}

void
mptcp_update_rcv_state_meat(struct mptcb *mp_tp, struct tcpcb *tp,
    u_int64_t full_dsn, u_int32_t seqn, u_int16_t mdss_data_len,
    uint16_t csum)
{
	if (mdss_data_len == 0) {
		os_log_error(mptcp_log_handle, "%s - %lx: Infinite Mapping.\n",
		    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mp_tp->mpt_mpte));

		if ((mp_tp->mpt_flags & MPTCPF_CHECKSUM) && (csum != 0)) {
			os_log_error(mptcp_log_handle, "%s - %lx: Bad checksum %x \n",
			    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mp_tp->mpt_mpte), csum);
		}
		mptcp_notify_mpfail(tp->t_inpcb->inp_socket);
		return;
	}

	mptcp_notify_mpready(tp->t_inpcb->inp_socket);

	tp->t_rcv_map.mpt_dsn = full_dsn;
	tp->t_rcv_map.mpt_sseq = seqn;
	tp->t_rcv_map.mpt_len = mdss_data_len;
	tp->t_rcv_map.mpt_csum = csum;
	tp->t_mpflags |= TMPF_EMBED_DSN;
}


static int
mptcp_validate_dss_map(struct socket *so, struct tcpcb *tp, struct mbuf *m,
    int hdrlen)
{
	u_int32_t datalen;

	if (!(m->m_pkthdr.pkt_flags & PKTF_MPTCP)) {
		return 0;
	}

	datalen = m->m_pkthdr.mp_rlen;

	                                              
	if (m->m_pkthdr.len > ((int) datalen + hdrlen)) {
		os_log_error(mptcp_log_handle, "%s - %lx: mbuf len %d, MPTCP expected %d",
		    __func__, (unsigned long)VM_KERNEL_ADDRPERM(tptomptp(tp)->mpt_mpte), m->m_pkthdr.len, datalen);
	} else {
		return 0;
	}
	tp->t_mpflags |= TMPF_SND_MPFAIL;
	mptcp_notify_mpfail(so);
	m_freem(m);
	return -1;
}

int
mptcp_input_preproc(struct tcpcb *tp, struct mbuf *m, struct tcphdr *th,
    int drop_hdrlen)
{
	mptcp_insert_rmap(tp, m, th);
	if (mptcp_validate_dss_map(tp->t_inpcb->inp_socket, tp, m,
	    drop_hdrlen) != 0) {
		return -1;
	}
	return 0;
}

static uint16_t
mptcp_input_csum(struct tcpcb *tp, struct mbuf *m, uint64_t dsn, uint32_t sseq,
    uint16_t dlen, uint16_t csum, int dfin)
{
	struct mptcb *mp_tp = tptomptp(tp);
	int real_len = dlen - dfin;
	uint32_t sum = 0;

	VERIFY(real_len >= 0);

	if (mp_tp == NULL) {
		return 0;
	}

	if (!(mp_tp->mpt_flags & MPTCPF_CHECKSUM)) {
		return 0;
	}

	if (tp->t_mpflags & TMPF_TCP_FALLBACK) {
		return 0;
	}

	  
                                                               
                                
    
	if ((int)m_length2(m, NULL) < real_len) {
		return 0xffff;
	}

	if (real_len != 0) {
		sum = m_sum16(m, 0, real_len);
	}

	sum += in_pseudo64(htonll(dsn), htonl(sseq), htons(dlen) + csum);
	ADDCARRY(sum);

	DTRACE_MPTCP3(checksum__result, struct tcpcb *, tp, struct mbuf *, m,
	    uint32_t, sum);

	return ~sum & 0xffff;
}

  
                         
                                                                       
                                                                       
                                                                      
              
   

int
mptcp_validate_csum(struct tcpcb *tp, struct mbuf *m, uint64_t dsn,
    uint32_t sseq, uint16_t dlen, uint16_t csum, int dfin)
{
	uint16_t mptcp_csum;

	mptcp_csum = mptcp_input_csum(tp, m, dsn, sseq, dlen, csum, dfin);
	if (mptcp_csum) {
		tp->t_mpflags |= TMPF_SND_MPFAIL;
		mptcp_notify_mpfail(tp->t_inpcb->inp_socket);
		m_freem(m);
		tcpstat.tcps_mp_badcsum++;
		return -1;
	}
	return 0;
}

uint16_t
mptcp_output_csum(struct mbuf *m, uint64_t dss_val, uint32_t sseq, uint16_t dlen)
{
	uint32_t sum = 0;

	if (dlen) {
		sum = m_sum16(m, 0, dlen);
	}

	dss_val = mptcp_hton64(dss_val);
	sseq = htonl(sseq);
	dlen = htons(dlen);
	sum += in_pseudo64(dss_val, sseq, dlen);

	ADDCARRY(sum);
	sum = ~sum & 0xffff;
	DTRACE_MPTCP2(checksum__result, struct mbuf *, m, uint32_t, sum);
	mptcplog((LOG_DEBUG, "%s: sum = %x \n", __func__, sum),
	    MPTCP_SENDER_DBG, MPTCP_LOGLVL_VERBOSE);

	return (uint16_t)sum;
}

  
                                                                    
                                                             
                                   
   
boolean_t
mptcp_no_rto_spike(struct socket *so)
{
	struct tcpcb *tp = intotcpcb(sotoinpcb(so));
	int32_t spike = 0;

	if (tp->t_rxtcur > mptcp_rtothresh) {
		spike = tp->t_rxtcur - mptcp_rtothresh;

		mptcplog((LOG_DEBUG, "%s: spike = %d rto = %d best = %d cur = %d\n",
		    __func__, spike,
		    tp->t_rxtcur, tp->t_rttbest >> TCP_RTT_SHIFT,
		    tp->t_rttcur),
		    (MPTCP_SOCKET_DBG | MPTCP_SENDER_DBG), MPTCP_LOGLVL_LOG);
	}

	if (spike > 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void
mptcp_handle_deferred_upcalls(struct mppcb *mpp, uint32_t flag)
{
	VERIFY(mpp->mpp_flags & flag);
	mpp->mpp_flags &= ~flag;

	if (mptcp_should_defer_upcall(mpp)) {
		return;
	}

	if (mpp->mpp_flags & MPP_SHOULD_WORKLOOP) {
		mpp->mpp_flags &= ~MPP_SHOULD_WORKLOOP;

		mptcp_subflow_workloop(mpp->mpp_pcbe);
	}

	if (mpp->mpp_flags & MPP_SHOULD_RWAKEUP) {
		mpp->mpp_flags &= ~MPP_SHOULD_RWAKEUP;

		sorwakeup(mpp->mpp_socket);
	}

	if (mpp->mpp_flags & MPP_SHOULD_WWAKEUP) {
		mpp->mpp_flags &= ~MPP_SHOULD_WWAKEUP;

		sowwakeup(mpp->mpp_socket);
	}
}

static void
mptcp_reset_itfinfo(struct mpt_itf_info *info)
{
	memset(info, 0, sizeof(*info));
}

void
mptcp_session_necp_cb(void *handle, int action, uint32_t interface_index,
    uint32_t necp_flags, __unused bool *viable)
{
	boolean_t has_v4 = !!(necp_flags & NECP_CLIENT_RESULT_FLAG_HAS_IPV4);
	boolean_t has_v6 = !!(necp_flags & NECP_CLIENT_RESULT_FLAG_HAS_IPV6);
	boolean_t has_nat64 = !!(necp_flags & NECP_CLIENT_RESULT_FLAG_HAS_NAT64);
	boolean_t low_power = !!(necp_flags & NECP_CLIENT_RESULT_FLAG_INTERFACE_LOW_POWER);
	struct mppcb *mp = (struct mppcb *)handle;
	struct mptses *mpte = mptompte(mp);
	struct socket *mp_so;
	struct mptcb *mp_tp;
	uint32_t i, ifindex;
	struct ifnet *ifp;
	int locked = 0;

	ifindex = interface_index;
	VERIFY(ifindex != IFSCOPE_NONE);

	                                                                            
	if (mp->mpp_socket->so_usecount == 0) {
		return;
	}

	mp_so = mptetoso(mpte);

	if (action != NECP_CLIENT_CBACTION_INITIAL) {
		socket_lock(mp_so, 1);
		locked = 1;

		                                                              
		if (mp->mpp_socket->so_usecount == 0) {
			goto out;
		}
	}

	socket_lock_assert_owned(mp_so);

	mp_tp = mpte->mpte_mptcb;

	ifnet_head_lock_shared();
	ifp = ifindex2ifnet[ifindex];
	ifnet_head_done();

	os_log(mptcp_log_handle, "%s - %lx: action: %u ifindex %u delegated to %u usecount %u mpt_flags %#x state %u v4 %u v6 %u nat64 %u power %u\n",
	    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mpte), action, ifindex,
	    ifp && ifp->if_delegated.ifp ? ifp->if_delegated.ifp->if_index : IFSCOPE_NONE,
	    mp->mpp_socket->so_usecount, mp_tp->mpt_flags, mp_tp->mpt_state,
	    has_v4, has_v6, has_nat64, low_power);

	                                    
	if (mp_tp->mpt_flags & MPTCPF_FALLBACK_TO_TCP) {
		goto out;
	}

	  
                                                                        
                                                               
    
	if (low_power) {
		action = NECP_CLIENT_CBACTION_NONVIABLE;
	}

	if (action == NECP_CLIENT_CBACTION_NONVIABLE) {
		for (i = 0; i < mpte->mpte_itfinfo_size; i++) {
			if (mpte->mpte_itfinfo[i].ifindex == IFSCOPE_NONE) {
				continue;
			}

			if (mpte->mpte_itfinfo[i].ifindex == ifindex) {
				mptcp_reset_itfinfo(&mpte->mpte_itfinfo[i]);
			}
		}

		mptcp_sched_create_subflows(mpte);
	} else if (action == NECP_CLIENT_CBACTION_VIABLE ||
	    action == NECP_CLIENT_CBACTION_INITIAL) {
		int found_slot = 0, slot_index = -1;
		struct sockaddr *dst;

		if (ifp == NULL) {
			goto out;
		}

		if (IFNET_IS_COMPANION_LINK(ifp)) {
			goto out;
		}

		if (IFNET_IS_EXPENSIVE(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_EXPENSIVE)) {
			goto out;
		}

		if (IFNET_IS_CONSTRAINED(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_CONSTRAINED)) {
			goto out;
		}

		if (IFNET_IS_CELLULAR(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_CELLULAR)) {
			goto out;
		}

		if (IS_INTF_CLAT46(ifp)) {
			has_v4 = FALSE;
		}

		                                                                    
		for (i = 0; i < mpte->mpte_itfinfo_size; i++) {
			                                                      
			if (mpte->mpte_itfinfo[i].ifindex == 0) {
				found_slot = 1;
				slot_index = i;
			}

			  
                                                        
                        
      
			if (mpte->mpte_itfinfo[i].ifindex == ifindex &&
			    (mpte->mpte_itfinfo[i].has_v4_conn != has_v4 ||
			    mpte->mpte_itfinfo[i].has_v6_conn != has_v6 ||
			    mpte->mpte_itfinfo[i].has_nat64_conn != has_nat64)) {
				found_slot = 1;
				slot_index = i;
				break;
			}

			if (mpte->mpte_itfinfo[i].ifindex == ifindex) {
				  
                                               
                   
       
				goto out;
			}
		}

		dst = mptcp_get_session_dst(mpte, has_v6, has_v4);
		if (dst && dst->sa_family == AF_INET &&
		    has_v6 && !has_nat64 && !has_v4) {
			if (found_slot) {
				mpte->mpte_itfinfo[slot_index].ifindex = ifindex;
				mpte->mpte_itfinfo[slot_index].has_v4_conn = has_v4;
				mpte->mpte_itfinfo[slot_index].has_v6_conn = has_v6;
				mpte->mpte_itfinfo[slot_index].has_nat64_conn = has_nat64;
			}
			goto out;
		}

		if (found_slot == 0) {
			int new_size = mpte->mpte_itfinfo_size * 2;
			struct mpt_itf_info *info = _MALLOC(sizeof(*info) * new_size, M_TEMP, M_ZERO);

			if (info == NULL) {
				os_log_error(mptcp_log_handle, "%s - %lx: malloc failed for %u\n",
				    __func__, (unsigned long)VM_KERNEL_ADDRPERM(mpte), new_size);
				goto out;
			}

			memcpy(info, mpte->mpte_itfinfo, mpte->mpte_itfinfo_size * sizeof(*info));

			if (mpte->mpte_itfinfo_size > MPTE_ITFINFO_SIZE) {
				_FREE(mpte->mpte_itfinfo, M_TEMP);
			}

			                                                          
			slot_index = mpte->mpte_itfinfo_size;

			mpte->mpte_itfinfo = info;
			mpte->mpte_itfinfo_size = new_size;
		}

		VERIFY(slot_index >= 0 && slot_index < (int)mpte->mpte_itfinfo_size);
		mpte->mpte_itfinfo[slot_index].ifindex = ifindex;
		mpte->mpte_itfinfo[slot_index].has_v4_conn = has_v4;
		mpte->mpte_itfinfo[slot_index].has_v6_conn = has_v6;
		mpte->mpte_itfinfo[slot_index].has_nat64_conn = has_nat64;

		mptcp_sched_create_subflows(mpte);
	}

out:
	if (locked) {
		socket_unlock(mp_so, 1);
	}
}

void
mptcp_set_restrictions(struct socket *mp_so)
{
	struct mptses *mpte = mpsotompte(mp_so);
	uint32_t i;

	socket_lock_assert_owned(mp_so);

	ifnet_head_lock_shared();

	for (i = 0; i < mpte->mpte_itfinfo_size; i++) {
		struct mpt_itf_info *info = &mpte->mpte_itfinfo[i];
		uint32_t ifindex = info->ifindex;
		struct ifnet *ifp;

		if (ifindex == IFSCOPE_NONE) {
			continue;
		}

		ifp = ifindex2ifnet[ifindex];
		if (ifp == NULL) {
			continue;
		}

		if (IFNET_IS_EXPENSIVE(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_EXPENSIVE)) {
			info->ifindex = IFSCOPE_NONE;
		}

		if (IFNET_IS_CONSTRAINED(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_CONSTRAINED)) {
			info->ifindex = IFSCOPE_NONE;
		}

		if (IFNET_IS_CELLULAR(ifp) &&
		    (mp_so->so_restrictions & SO_RESTRICT_DENY_CELLULAR)) {
			info->ifindex = IFSCOPE_NONE;
		}
	}

	ifnet_head_done();
}
