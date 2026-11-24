/*
 * Copyright (c) 2004-2016 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
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
 */

#define _IP_VHL


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <kern/zalloc.h>

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_cache.h>
#if TCPDEBUG
#include <netinet/tcp_debug.h>
#endif
#include <sys/kdebug.h>

#if IPSEC
#include <netinet6/ipsec.h>
#endif          

#include <libkern/OSAtomic.h>

SYSCTL_SKMEM_TCP_INT(OID_AUTO, sack, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_do_sack, 1, "Enable/Disable TCP SACK support");
SYSCTL_SKMEM_TCP_INT(OID_AUTO, sack_maxholes, CTLFLAG_RW | CTLFLAG_LOCKED,
    static int, tcp_sack_maxholes, 128,
    "Maximum number of TCP SACK holes allowed per connection");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, sack_globalmaxholes,
    CTLFLAG_RW | CTLFLAG_LOCKED, static int, tcp_sack_globalmaxholes, 65536,
    "Global maximum number of TCP SACK holes");

static SInt32 tcp_sack_globalholes = 0;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, sack_globalholes, CTLFLAG_RD | CTLFLAG_LOCKED,
    &tcp_sack_globalholes, 0,
    "Global number of TCP SACK holes currently allocated");

extern struct zone *sack_hole_zone;

#define TCP_VALIDATE_SACK_SEQ_NUMBERS(_tp_, _sb_, _ack_) \
    (SEQ_GT((_sb_)->end, (_sb_)->start) && \
    SEQ_GT((_sb_)->start, (_tp_)->snd_una) && \
    SEQ_GT((_sb_)->start, (_ack_)) && \
    SEQ_LT((_sb_)->start, (_tp_)->snd_max) && \
    SEQ_GT((_sb_)->end, (_tp_)->snd_una) && \
    SEQ_LEQ((_sb_)->end, (_tp_)->snd_max))

  
                                                                              
                                                              
   
void
tcp_update_sack_list(struct tcpcb *tp, tcp_seq rcv_start, tcp_seq rcv_end)
{
	  
                                                                 
                                                              
                                                                 
                            
    
	struct sackblk head_blk, saved_blks[MAX_SACK_BLKS];
	int num_head, num_saved, i;

	                                          
	head_blk.start = rcv_start;
	head_blk.end = rcv_end;

	  
                                                
                                                 
                                                            
    
	num_saved = 0;
	for (i = 0; i < tp->rcv_numsacks; i++) {
		tcp_seq start = tp->sackblks[i].start;
		tcp_seq end = tp->sackblks[i].end;
		if (SEQ_GEQ(start, end) || SEQ_LEQ(start, tp->rcv_nxt)) {
			  
                              
      
		} else if (SEQ_LEQ(head_blk.start, end) &&
		    SEQ_GEQ(head_blk.end, start)) {
			  
                                          
                                               
      
			if (SEQ_GT(head_blk.start, start)) {
				head_blk.start = start;
			}
			if (SEQ_LT(head_blk.end, end)) {
				head_blk.end = end;
			}
		} else {
			  
                           
      
			saved_blks[num_saved].start = start;
			saved_blks[num_saved].end = end;
			num_saved++;
		}
	}

	  
                                       
    
	num_head = 0;
	if (SEQ_GT(head_blk.start, tp->rcv_nxt)) {
		  
                                                          
                                          
     
		tp->sackblks[0] = head_blk;
		num_head = 1;
		  
                                                          
                                 
     
		if (num_saved >= MAX_SACK_BLKS) {
			num_saved--;
		}
	}
	if (num_saved > 0) {
		  
                                     
     
		bcopy(saved_blks, &tp->sackblks[num_head], sizeof(struct sackblk) * num_saved);
	}

	                                     
	tp->rcv_numsacks = num_head + num_saved;

	                                                                  
                                                                 
                            
    
	if ((tp->t_flags & TF_STRETCHACK) != 0 && tp->rcv_numsacks > 0) {
		tcp_reset_stretch_ack(tp);
	}
	if (tp->rcv_numsacks > 0) {
		tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;
	}

#if TRAFFIC_MGT
	if (tp->acc_iaj > 0 && tp->rcv_numsacks > 0) {
		reset_acc_iaj(tp);
	}
#endif                  
}

  
                                             
   
void
tcp_clean_sackreport( struct tcpcb *tp)
{
	tp->rcv_numsacks = 0;
	bzero(&tp->sackblks[0], sizeof(struct sackblk) * MAX_SACK_BLKS);
}

  
                            
   
static struct sackhole *
tcp_sackhole_alloc(struct tcpcb *tp, tcp_seq start, tcp_seq end)
{
	struct sackhole *hole;

	if (tp->snd_numholes >= tcp_sack_maxholes ||
	    tcp_sack_globalholes >= tcp_sack_globalmaxholes) {
		tcpstat.tcps_sack_sboverflow++;
		return NULL;
	}

	hole = (struct sackhole *)zalloc(sack_hole_zone);
	if (hole == NULL) {
		return NULL;
	}

	hole->start = start;
	hole->end = end;
	hole->rxmit = start;

	tp->snd_numholes++;
	OSIncrementAtomic(&tcp_sack_globalholes);

	return hole;
}

  
                        
   
static void
tcp_sackhole_free(struct tcpcb *tp, struct sackhole *hole)
{
	zfree(sack_hole_zone, hole);

	tp->snd_numholes--;
	OSDecrementAtomic(&tcp_sack_globalholes);
}

  
                                        
   
static struct sackhole *
tcp_sackhole_insert(struct tcpcb *tp, tcp_seq start, tcp_seq end,
    struct sackhole *after)
{
	struct sackhole *hole;

	                               
	hole = tcp_sackhole_alloc(tp, start, end);
	if (hole == NULL) {
		return NULL;
	}
	hole->rxmit_start = tcp_now;
	                                              
	if (after != NULL) {
		TAILQ_INSERT_AFTER(&tp->snd_holes, after, hole, scblink);
	} else {
		TAILQ_INSERT_TAIL(&tp->snd_holes, hole, scblink);
	}

	                       
	if (tp->sackhint.nexthole == NULL) {
		tp->sackhint.nexthole = hole;
	}

	return hole;
}

  
                                    
   
static void
tcp_sackhole_remove(struct tcpcb *tp, struct sackhole *hole)
{
	                       
	if (tp->sackhint.nexthole == hole) {
		tp->sackhint.nexthole = TAILQ_NEXT(hole, scblink);
	}

	                            
	TAILQ_REMOVE(&tp->snd_holes, hole, scblink);

	                          
	tcp_sackhole_free(tp, hole);
}
  
                                                                     
                                                                      
                                                              
                                                          
   
static void
tcp_sack_detect_reordering(struct tcpcb *tp, struct sackhole *s,
    tcp_seq sacked_seq, tcp_seq snd_fack)
{
	int32_t rext = 0, reordered = 0;

	  
                                                            
                                     
    
	if (SEQ_GT(s->end, snd_fack)) {
		return;
	}
	  
                                                                 
                                                        
                                                             
    
	if (tp->t_rxtshift > 0) {
		return;
	}

	  
                                                       
                                                                   
   
                                                                               
                                                                            
   
                                                                           
                                  
    
	if (SEQ_LT(s->rxmit, sacked_seq) &&
	    (!tcp_do_better_lr || tcp_rxtseg_find(tp, sacked_seq - 1, sacked_seq - 1) == NULL)) {
		reordered = 1;
		tcpstat.tcps_avoid_rxmt++;
	}

	if (reordered) {
		if (!(tp->t_flagsext & TF_PKTS_REORDERED)) {
			tp->t_flagsext |= TF_PKTS_REORDERED;
			tcpstat.tcps_detect_reordering++;
		}

		tcpstat.tcps_reordered_pkts++;
		tp->t_reordered_pkts++;

		  
                                                           
                            
     
		if (TCP_ECN_ENABLED(tp)) {
			INP_INC_IFNET_STAT(tp->t_inpcb, ecn_fallback_reorder);
			tcpstat.tcps_ecn_fallback_reorder++;
			tcp_heuristic_ecn_aggressive(tp);
		}

		VERIFY(SEQ_GEQ(snd_fack, s->rxmit));

		if (s->rxmit_start > 0) {
			rext = timer_diff(tcp_now, 0, s->rxmit_start, 0);
			if (rext < 0) {
				return;
			}

			  
                                                    
                                                    
                          
     
                                                    
                                                     
                     
     
                                                 
                                  
      
			tp->t_reorderwin = max(tp->t_reorderwin, rext);
			tp->t_reorderwin = min(tp->t_reorderwin,
			    (tp->t_srtt >> (TCP_RTT_SHIFT - 1)));
			tp->t_reorderwin = max(tp->t_reorderwin, 10);
		}
	}
}

static void
tcp_sack_update_byte_counter(struct tcpcb *tp, uint32_t start, uint32_t end,
    uint32_t *newbytes_acked, uint32_t *towards_fr_acked)
{
	*newbytes_acked += (end - start);
	if (SEQ_GEQ(start, tp->send_highest_sack)) {
		*towards_fr_acked += (end - start);
	}
}

  
                                                                           
                                                                           
                       
   
void
tcp_sack_doack(struct tcpcb *tp, struct tcpopt *to, struct tcphdr *th,
    u_int32_t *newbytes_acked, uint32_t *after_rexmit_acked)
{
	struct sackhole *cur, *temp;
	struct sackblk sack, sack_blocks[TCP_MAX_SACK + 1], *sblkp;
	int i, j, num_sack_blks;
	tcp_seq old_snd_fack = 0, th_ack = th->th_ack;

	num_sack_blks = 0;
	  
                                                                    
                                                      
    
	if (SEQ_LT(tp->snd_una, th_ack) && !TAILQ_EMPTY(&tp->snd_holes)) {
		sack_blocks[num_sack_blks].start = tp->snd_una;
		sack_blocks[num_sack_blks++].end = th_ack;
	}
	  
                                                       
                                             
    
	for (i = 0; i < to->to_nsacks; i++) {
		bcopy((to->to_sacks + i * TCPOLEN_SACK),
		    &sack, sizeof(sack));
		sack.start = ntohl(sack.start);
		sack.end = ntohl(sack.end);
		if (TCP_VALIDATE_SACK_SEQ_NUMBERS(tp, &sack, th_ack)) {
			sack_blocks[num_sack_blks++] = sack;
		}
	}

	  
                                                             
                
    
	if (num_sack_blks == 0) {
		return;
	}

	VERIFY(num_sack_blks <= (TCP_MAX_SACK + 1));
	  
                                                        
                                                                 
                                                            
    
	for (i = 0; i < num_sack_blks; i++) {
		for (j = i + 1; j < num_sack_blks; j++) {
			if (SEQ_GT(sack_blocks[i].end, sack_blocks[j].end)) {
				sack = sack_blocks[i];
				sack_blocks[i] = sack_blocks[j];
				sack_blocks[j] = sack;
			}
		}
	}
	if (TAILQ_EMPTY(&tp->snd_holes)) {
		  
                                                             
                                                           
                                                                
                                                              
     
		tp->snd_fack = SEQ_MAX(tp->snd_una, th_ack);
	}

	old_snd_fack = tp->snd_fack;
	  
                                                                 
                                                                  
                                                                      
                                              
                                                                   
                                                                    
                                                                   
                                                                   
                 
    
	sblkp = &sack_blocks[num_sack_blks - 1];                             
	if (SEQ_LT(tp->snd_fack, sblkp->start)) {
		  
                                           
                                      
                                                        
                                                      
                                                   
     
		temp = tcp_sackhole_insert(tp, tp->snd_fack, sblkp->start, NULL);
		if (temp != NULL) {
			tp->snd_fack = sblkp->end;
			tcp_sack_update_byte_counter(tp, sblkp->start, sblkp->end, newbytes_acked, after_rexmit_acked);

			                                    
			sblkp--;
		} else {
			  
                                                      
                                                     
                                                          
                                                        
                                                       
                                
      
			while (sblkp >= sack_blocks &&
			    SEQ_LT(tp->snd_fack, sblkp->start)) {
				sblkp--;
			}
			if (sblkp >= sack_blocks &&
			    SEQ_LT(tp->snd_fack, sblkp->end)) {
				tcp_sack_update_byte_counter(tp, tp->snd_fack, sblkp->end, newbytes_acked, after_rexmit_acked);
				tp->snd_fack = sblkp->end;
			}
		}
	} else if (SEQ_LT(tp->snd_fack, sblkp->end)) {
		                       
		tcp_sack_update_byte_counter(tp, tp->snd_fack, sblkp->end, newbytes_acked, after_rexmit_acked);
		tp->snd_fack = sblkp->end;
	}
	                                                       
	cur = TAILQ_LAST(&tp->snd_holes, sackhole_head);                     
	  
                                                                  
                                       
    
	while (sblkp >= sack_blocks && cur != NULL) {
		if (SEQ_GEQ(sblkp->start, cur->end)) {
			  
                                         
                                    
      
			sblkp--;
			continue;
		}
		if (SEQ_LEQ(sblkp->end, cur->start)) {
			  
                                         
                              
      
			cur = TAILQ_PREV(cur, sackhole_head, scblink);
			continue;
		}
		tp->sackhint.sack_bytes_rexmit -= (cur->rxmit - cur->start);
		if (tp->sackhint.sack_bytes_rexmit < 0) {
			tp->sackhint.sack_bytes_rexmit = 0;
		}

		if (SEQ_LEQ(sblkp->start, cur->start)) {
			                                              
			if (SEQ_GEQ(sblkp->end, cur->end)) {
				                                      
				tcp_sack_update_byte_counter(tp, cur->start, cur->end, newbytes_acked, after_rexmit_acked);

				tcp_sack_detect_reordering(tp, cur,
				    cur->end, old_snd_fack);
				temp = cur;
				cur = TAILQ_PREV(cur, sackhole_head, scblink);
				tcp_sackhole_remove(tp, temp);
				  
                                                     
                                                
       
				continue;
			} else {
				                                
				tcp_sack_update_byte_counter(tp, cur->start, sblkp->end, newbytes_acked, after_rexmit_acked);
				tcp_sack_detect_reordering(tp, cur,
				    sblkp->end, old_snd_fack);
				cur->start = sblkp->end;
				cur->rxmit = SEQ_MAX(cur->rxmit, cur->start);
			}
		} else {
			                                        
			if (SEQ_GEQ(sblkp->end, cur->end)) {
				                               
				tcp_sack_update_byte_counter(tp, sblkp->start, cur->end, newbytes_acked, after_rexmit_acked);
				tcp_sack_detect_reordering(tp, cur,
				    cur->end, old_snd_fack);
				cur->end = sblkp->start;
				cur->rxmit = SEQ_MIN(cur->rxmit, cur->end);
			} else {
				  
                                              
                                 
       
				tcp_sack_detect_reordering(tp, cur,
				    sblkp->end, old_snd_fack);
				temp = tcp_sackhole_insert(tp, sblkp->end,
				    cur->end, cur);
				if (temp != NULL) {
					tcp_sack_update_byte_counter(tp, sblkp->start, sblkp->end, newbytes_acked, after_rexmit_acked);
					if (SEQ_GT(cur->rxmit, temp->rxmit)) {
						temp->rxmit = cur->rxmit;
						tp->sackhint.sack_bytes_rexmit
						        += (temp->rxmit
						    - temp->start);
					}
					cur->end = sblkp->start;
					cur->rxmit = SEQ_MIN(cur->rxmit,
					    cur->end);
					  
                                        
                                     
                                   
                        
        
					temp->rxmit_start = cur->rxmit_start;
				}
			}
		}
		tp->sackhint.sack_bytes_rexmit += (cur->rxmit - cur->start);
		  
                                                             
                                                     
                                              
     
		if (SEQ_LEQ(sblkp->start, cur->start)) {
			cur = TAILQ_PREV(cur, sackhole_head, scblink);
		} else {
			sblkp--;
		}
	}
}

  
                                               
   
void
tcp_free_sackholes(struct tcpcb *tp)
{
	struct sackhole *q;

	while ((q = TAILQ_FIRST(&tp->snd_holes)) != NULL) {
		tcp_sackhole_remove(tp, q);
	}
	tp->sackhint.sack_bytes_rexmit = 0;
	tp->sackhint.sack_bytes_acked = 0;
	tp->t_new_dupacks = 0;
	tp->sackhint.nexthole = NULL;
	tp->sack_newdata = 0;
}

  
                                                       
                                                       
                                                         
                                                  
                                                    
                                                       
                                                           
                    
   
void
tcp_sack_partialack(struct tcpcb *tp, struct tcphdr *th)
{
	int num_segs = 1;

	tp->t_timer[TCPT_REXMT] = 0;
	tp->t_rtttime = 0;
	                                                                 
	if (((BYTES_ACKED(th, tp)) / tp->t_maxseg) > 2) {
		num_segs = 2;
	}
	if (tcp_do_better_lr) {
		tp->snd_cwnd = tcp_flight_size(tp) + num_segs * tp->t_maxseg;
	} else {
		tp->snd_cwnd = (tp->sackhint.sack_bytes_rexmit +
		    (tp->snd_nxt - tp->sack_newdata) +
		    num_segs * tp->t_maxseg);
	}
	if (tp->snd_cwnd > tp->snd_ssthresh) {
		tp->snd_cwnd = tp->snd_ssthresh;
	}
	if (SEQ_LT(tp->snd_fack, tp->snd_recover) &&
	    tp->snd_fack == th->th_ack && TAILQ_EMPTY(&tp->snd_holes)) {
		struct sackhole *temp;
		  
                                                        
                                                           
                                                       
                                     
     
		temp = tcp_sackhole_insert(tp, tp->snd_fack,
		    tp->snd_recover, NULL);
		if (temp != NULL) {
			tp->snd_fack = tp->snd_recover;
		}
	}
	(void) tcp_output(tp);
}

  
                                                                         
                                
   
static struct sackhole *
tcp_sack_output_debug(struct tcpcb *tp, int *sack_bytes_rexmt)
{
	struct sackhole *p;

	*sack_bytes_rexmt = 0;
	TAILQ_FOREACH(p, &tp->snd_holes, scblink) {
		if (SEQ_LT(p->rxmit, p->end)) {
			if (SEQ_LT(p->rxmit, tp->snd_una)) {                   
				continue;
			}
			*sack_bytes_rexmt += (p->rxmit - p->start);
			break;
		}
		*sack_bytes_rexmt += (p->rxmit - p->start);
	}
	return p;
}

  
                                                                            
                                                                     
                                                                             
                                                            
  
                                                                       
                                                                            
                                                                               
                                                                      
                                                                              
                                                                               
                                                                    
                                                                             
                                                                            
              
   
struct sackhole *
tcp_sack_output(struct tcpcb *tp, int *sack_bytes_rexmt)
{
	struct sackhole *hole = NULL, *dbg_hole = NULL;
	int dbg_bytes_rexmt;

	dbg_hole = tcp_sack_output_debug(tp, &dbg_bytes_rexmt);
	*sack_bytes_rexmt = tp->sackhint.sack_bytes_rexmit;
	hole = tp->sackhint.nexthole;
	if (hole == NULL || SEQ_LT(hole->rxmit, hole->end)) {
		goto out;
	}
	while ((hole = TAILQ_NEXT(hole, scblink)) != NULL) {
		if (SEQ_LT(hole->rxmit, hole->end)) {
			tp->sackhint.nexthole = hole;
			break;
		}
	}
out:
	if (dbg_hole != hole) {
		printf("%s: Computed sack hole not the same as cached value\n", __func__);
		hole = dbg_hole;
	}
	if (*sack_bytes_rexmt != dbg_bytes_rexmt) {
		printf("%s: Computed sack_bytes_retransmitted (%d) not "
		    "the same as cached value (%d)\n",
		    __func__, dbg_bytes_rexmt, *sack_bytes_rexmt);
		*sack_bytes_rexmt = dbg_bytes_rexmt;
	}
	return hole;
}

void
tcp_sack_lost_rexmit(struct tcpcb *tp)
{
	struct sackhole *hole = TAILQ_FIRST(&tp->snd_holes);

	while (hole) {
		hole->rxmit = hole->start;
		hole->rxmit_start = tcp_now;

		hole = TAILQ_NEXT(hole, scblink);
	}

	tp->sackhint.nexthole = TAILQ_FIRST(&tp->snd_holes);
	tp->sackhint.sack_bytes_rexmit = 0;
	tp->sack_newdata = tp->snd_nxt;
}

  
                                                                        
                                                                     
                                                                     
   
void
tcp_sack_adjust(struct tcpcb *tp)
{
	struct sackhole *p, *cur = TAILQ_FIRST(&tp->snd_holes);

	if (cur == NULL) {
		return;               
	}
	if (SEQ_GEQ(tp->snd_nxt, tp->snd_fack)) {
		return;                                             
	}
	  
                                                   
                                                                    
                                                          
    
	while ((p = TAILQ_NEXT(cur, scblink)) != NULL) {
		if (SEQ_LT(tp->snd_nxt, cur->end)) {
			return;
		}
		if (SEQ_GEQ(tp->snd_nxt, p->start)) {
			cur = p;
		} else {
			tp->snd_nxt = p->start;
			return;
		}
	}
	if (SEQ_LT(tp->snd_nxt, cur->end)) {
		return;
	}
	tp->snd_nxt = tp->snd_fack;
	return;
}

  
                                                                      
                                                                     
   
boolean_t
tcp_sack_byte_islost(struct tcpcb *tp)
{
	u_int32_t unacked_bytes, sndhole_bytes = 0;
	struct sackhole *sndhole;
	if (!SACK_ENABLED(tp) || IN_FASTRECOVERY(tp) ||
	    TAILQ_EMPTY(&tp->snd_holes) ||
	    (tp->t_flagsext & TF_PKTS_REORDERED)) {
		return FALSE;
	}

	unacked_bytes = tp->snd_max - tp->snd_una;

	TAILQ_FOREACH(sndhole, &tp->snd_holes, scblink) {
		sndhole_bytes += (sndhole->end - sndhole->start);
	}

	VERIFY(unacked_bytes >= sndhole_bytes);
	return (unacked_bytes - sndhole_bytes) >
	       ((tcprexmtthresh - 1) * tp->t_maxseg);
}

  
                                                                     
   

boolean_t
tcp_sack_process_dsack(struct tcpcb *tp, struct tcpopt *to,
    struct tcphdr *th)
{
	struct sackblk first_sack, second_sack;
	struct tcp_rxt_seg *rxseg;

	bcopy(to->to_sacks, &first_sack, sizeof(first_sack));
	first_sack.start = ntohl(first_sack.start);
	first_sack.end = ntohl(first_sack.end);

	if (to->to_nsacks > 1) {
		bcopy((to->to_sacks + TCPOLEN_SACK), &second_sack,
		    sizeof(second_sack));
		second_sack.start = ntohl(second_sack.start);
		second_sack.end = ntohl(second_sack.end);
	}

	if (SEQ_LT(first_sack.start, th->th_ack) &&
	    SEQ_LEQ(first_sack.end, th->th_ack)) {
		  
                                                          
                                                
    
                                                          
                                                      
                                                             
                                                        
                                                            
     
		if (!(TCP_DSACK_SEQ_IN_WINDOW(tp, first_sack.start, th->th_ack) &&
		    TCP_DSACK_SEQ_IN_WINDOW(tp, first_sack.end, th->th_ack))) {
			to->to_nsacks--;
			to->to_sacks += TCPOLEN_SACK;
			tcpstat.tcps_dsack_recvd_old++;

			  
                                                     
                               
      
			return TRUE;
		}
	} else if (to->to_nsacks > 1 &&
	    SEQ_LEQ(second_sack.start, first_sack.start) &&
	    SEQ_GEQ(second_sack.end, first_sack.end)) {
		  
                                                   
                                                          
                              
    
                                                        
                                                        
                                                
     
		if (!TCP_VALIDATE_SACK_SEQ_NUMBERS(tp, &second_sack,
		    th->th_ack)) {
			to->to_nsacks--;
			to->to_sacks += TCPOLEN_SACK;
			tcpstat.tcps_dsack_recvd_old++;
			return TRUE;
		}
	} else {
		                                                        
		return FALSE;
	}

	                                                      
	to->to_nsacks--;
	to->to_sacks += TCPOLEN_SACK;
	tcpstat.tcps_dsack_recvd++;
	tp->t_dsack_recvd++;

	                                             
	if ((tp->t_flagsext & TF_SENT_TLPROBE) &&
	    first_sack.end == tp->t_tlphighrxt) {
		if ((rxseg = tcp_rxtseg_find(tp, first_sack.start,
		    (first_sack.end - 1))) != NULL) {
			rxseg->rx_flags |= TCP_RXT_DSACK_FOR_TLP;
		}
	}
	                                                  
	if (((tp->t_rxtshift == 1 && first_sack.start == tp->snd_una) ||
	    ((tp->t_flagsext & TF_SENT_TLPROBE) &&
	    first_sack.end == tp->t_tlphighrxt)) &&
	    TAILQ_EMPTY(&tp->snd_holes) &&
	    SEQ_GT(th->th_ack, tp->snd_una)) {
		  
                                                          
                                                  
                                                         
                                                 
    
                                                       
             
     
		tcpstat.tcps_dsack_ackloss++;

		return TRUE;
	} else {
		tcp_rxtseg_set_spurious(tp, first_sack.start, (first_sack.end - 1));
	}
	return TRUE;
}
