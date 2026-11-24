/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1998-2002 Luigi Rizzo, Universita` di Pisa
 * Portions Copyright (c) 2000 Akamba Corp.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/netinet/ip_dummynet.c,v 1.84 2004/08/25 09:31:30 pjd Exp $
 */

#define DUMMYNET_DEBUG

  
                                                                         
                                                              
                                                     
                            
                                
                                       
                                       
  
                                                                     
  
                          
  
                               
                                
                       
                                                              
                          
  
                                                        
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/queue.h>                           
#include <sys/kernel.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>
#include <net/kpi_protocol.h>
#if DUMMYNET
#include <net/kpi_protocol.h>
#endif               
#include <net/nwk_wq.h>
#include <net/pfvar.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_dummynet.h>
#include <netinet/ip_var.h>

#include <netinet/ip6.h>                                                 
#include <netinet6/ip6_var.h>

  
                                                                   
                                                                      
   
static dn_key curr_time = 0;                               

                                                                                      
                                                                 
static int timer_enabled = 0;

static int dn_hash_size = 64;                          

                                                             
static int searches, search_steps;
static int pipe_expire = 1;                               
static int dn_max_ratio = 16;                                

static int red_lookup_depth = 256;                                            
static int red_avg_pkt_size = 512;                                            
static int red_max_pkt_size = 1500;                                        

static int serialize = 0;

  
                                                                   
  
                                                                     
  
                                                               
  
                                                           
  
   
static struct dn_heap ready_heap, extract_heap, wfq_ready_heap;

static int heap_init(struct dn_heap *h, int size);
static int heap_insert(struct dn_heap *h, dn_key key1, void *p);
static void heap_extract(struct dn_heap *h, void *obj);


static void     transmit_event(struct dn_pipe *pipe, struct mbuf **head,
    struct mbuf **tail);
static void     ready_event(struct dn_flow_queue *q, struct mbuf **head,
    struct mbuf **tail);
static void     ready_event_wfq(struct dn_pipe *p, struct mbuf **head,
    struct mbuf **tail);

  
                                                                     
                                                                      
                                      
   
static void dummynet_send(struct mbuf *m);

#define HASHSIZE        16
#define HASH(num)       ((((num) >> 8) ^ ((num) >> 4) ^ (num)) & 0x0f)
static struct dn_pipe_head      pipehash[HASHSIZE];                    
static struct dn_flow_set_head  flowsethash[HASHSIZE];                    

#ifdef SYSCTL_NODE
SYSCTL_NODE(_net_inet_ip, OID_AUTO, dummynet,
    CTLFLAG_RW | CTLFLAG_LOCKED, 0, "Dummynet");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, hash_size,
    CTLFLAG_RW | CTLFLAG_LOCKED, &dn_hash_size, 0, "Default hash table size");
SYSCTL_QUAD(_net_inet_ip_dummynet, OID_AUTO, curr_time,
    CTLFLAG_RD | CTLFLAG_LOCKED, &curr_time, "Current tick");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, ready_heap,
    CTLFLAG_RD | CTLFLAG_LOCKED, &ready_heap.size, 0, "Size of ready heap");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, extract_heap,
    CTLFLAG_RD | CTLFLAG_LOCKED, &extract_heap.size, 0, "Size of extract heap");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, searches,
    CTLFLAG_RD | CTLFLAG_LOCKED, &searches, 0, "Number of queue searches");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, search_steps,
    CTLFLAG_RD | CTLFLAG_LOCKED, &search_steps, 0, "Number of queue search steps");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, expire,
    CTLFLAG_RW | CTLFLAG_LOCKED, &pipe_expire, 0, "Expire queue if empty");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, max_chain_len,
    CTLFLAG_RW | CTLFLAG_LOCKED, &dn_max_ratio, 0,
    "Max ratio between dynamic queues and buckets");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, red_lookup_depth,
    CTLFLAG_RD | CTLFLAG_LOCKED, &red_lookup_depth, 0, "Depth of RED lookup table");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, red_avg_pkt_size,
    CTLFLAG_RD | CTLFLAG_LOCKED, &red_avg_pkt_size, 0, "RED Medium packet size");
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, red_max_pkt_size,
    CTLFLAG_RD | CTLFLAG_LOCKED, &red_max_pkt_size, 0, "RED Max packet size");
#endif

#ifdef DUMMYNET_DEBUG
int     dummynet_debug = 0;
#ifdef SYSCTL_NODE
SYSCTL_INT(_net_inet_ip_dummynet, OID_AUTO, debug, CTLFLAG_RW | CTLFLAG_LOCKED, &dummynet_debug,
    0, "control debugging printfs");
#endif
#define DPRINTF(X)      if (dummynet_debug) printf X
#else
#define DPRINTF(X)
#endif

                   
static lck_grp_t         *dn_mutex_grp;
static lck_grp_attr_t    *dn_mutex_grp_attr;
static lck_attr_t        *dn_mutex_attr;
decl_lck_mtx_data(static, dn_mutex_data);
static lck_mtx_t         *dn_mutex = &dn_mutex_data;

static int config_pipe(struct dn_pipe *p);
static int ip_dn_ctl(struct sockopt *sopt);

static void dummynet(void *);
static void dummynet_flush(void);
void dummynet_drain(void);
static ip_dn_io_t dummynet_io;

static void cp_flow_set_to_64_user(struct dn_flow_set *set, struct dn_flow_set_64 *fs_bp);
static void cp_queue_to_64_user( struct dn_flow_queue *q, struct dn_flow_queue_64 *qp);
static char *cp_pipe_to_64_user(struct dn_pipe *p, struct dn_pipe_64 *pipe_bp);
static char* dn_copy_set_64(struct dn_flow_set *set, char *bp);
static int cp_pipe_from_user_64( struct sockopt *sopt, struct dn_pipe *p );

static void cp_flow_set_to_32_user(struct dn_flow_set *set, struct dn_flow_set_32 *fs_bp);
static void cp_queue_to_32_user( struct dn_flow_queue *q, struct dn_flow_queue_32 *qp);
static char *cp_pipe_to_32_user(struct dn_pipe *p, struct dn_pipe_32 *pipe_bp);
static char* dn_copy_set_32(struct dn_flow_set *set, char *bp);
static int cp_pipe_from_user_32( struct sockopt *sopt, struct dn_pipe *p );

struct eventhandler_lists_ctxt dummynet_evhdlr_ctxt;

uint32_t
my_random(void)
{
	uint32_t val;
	read_frandom(&val, sizeof(val));
	val &= 0x7FFFFFFF;

	return val;
}

  
                             
  
                                                                         
                                                                    
  
                                                        
                                          
                                                                
                                                
                                   
   
#define HEAP_FATHER(x) ( ( (x) - 1 ) / 2 )
#define HEAP_LEFT(x) ( 2*(x) + 1 )
#define HEAP_IS_LEFT(x) ( (x) & 1 )
#define HEAP_RIGHT(x) ( 2*(x) + 2 )
#define HEAP_SWAP(a, b, buffer) { buffer = a ; a = b ; b = buffer ; }
#define HEAP_INCREMENT  15


int
cp_pipe_from_user_32( struct sockopt *sopt, struct dn_pipe *p )
{
	struct dn_pipe_32 user_pipe_32;
	int error = 0;

	error = sooptcopyin(sopt, &user_pipe_32, sizeof(struct dn_pipe_32), sizeof(struct dn_pipe_32));
	if (!error) {
		p->pipe_nr = user_pipe_32.pipe_nr;
		p->bandwidth = user_pipe_32.bandwidth;
		p->delay = user_pipe_32.delay;
		p->V = user_pipe_32.V;
		p->sum = user_pipe_32.sum;
		p->numbytes = user_pipe_32.numbytes;
		p->sched_time = user_pipe_32.sched_time;
		bcopy( user_pipe_32.if_name, p->if_name, IFNAMSIZ);
		p->ready = user_pipe_32.ready;

		p->fs.fs_nr = user_pipe_32.fs.fs_nr;
		p->fs.flags_fs = user_pipe_32.fs.flags_fs;
		p->fs.parent_nr = user_pipe_32.fs.parent_nr;
		p->fs.weight = user_pipe_32.fs.weight;
		p->fs.qsize = user_pipe_32.fs.qsize;
		p->fs.plr = user_pipe_32.fs.plr;
		p->fs.flow_mask = user_pipe_32.fs.flow_mask;
		p->fs.rq_size = user_pipe_32.fs.rq_size;
		p->fs.rq_elements = user_pipe_32.fs.rq_elements;
		p->fs.last_expired = user_pipe_32.fs.last_expired;
		p->fs.backlogged = user_pipe_32.fs.backlogged;
		p->fs.w_q = user_pipe_32.fs.w_q;
		p->fs.max_th = user_pipe_32.fs.max_th;
		p->fs.min_th = user_pipe_32.fs.min_th;
		p->fs.max_p = user_pipe_32.fs.max_p;
		p->fs.c_1 = user_pipe_32.fs.c_1;
		p->fs.c_2 = user_pipe_32.fs.c_2;
		p->fs.c_3 = user_pipe_32.fs.c_3;
		p->fs.c_4 = user_pipe_32.fs.c_4;
		p->fs.lookup_depth = user_pipe_32.fs.lookup_depth;
		p->fs.lookup_step = user_pipe_32.fs.lookup_step;
		p->fs.lookup_weight = user_pipe_32.fs.lookup_weight;
		p->fs.avg_pkt_size = user_pipe_32.fs.avg_pkt_size;
		p->fs.max_pkt_size = user_pipe_32.fs.max_pkt_size;
	}
	return error;
}


int
cp_pipe_from_user_64( struct sockopt *sopt, struct dn_pipe *p )
{
	struct dn_pipe_64 user_pipe_64;
	int error = 0;

	error = sooptcopyin(sopt, &user_pipe_64, sizeof(struct dn_pipe_64), sizeof(struct dn_pipe_64));
	if (!error) {
		p->pipe_nr = user_pipe_64.pipe_nr;
		p->bandwidth = user_pipe_64.bandwidth;
		p->delay = user_pipe_64.delay;
		p->V = user_pipe_64.V;
		p->sum = user_pipe_64.sum;
		p->numbytes = user_pipe_64.numbytes;
		p->sched_time = user_pipe_64.sched_time;
		bcopy( user_pipe_64.if_name, p->if_name, IFNAMSIZ);
		p->ready = user_pipe_64.ready;

		p->fs.fs_nr = user_pipe_64.fs.fs_nr;
		p->fs.flags_fs = user_pipe_64.fs.flags_fs;
		p->fs.parent_nr = user_pipe_64.fs.parent_nr;
		p->fs.weight = user_pipe_64.fs.weight;
		p->fs.qsize = user_pipe_64.fs.qsize;
		p->fs.plr = user_pipe_64.fs.plr;
		p->fs.flow_mask = user_pipe_64.fs.flow_mask;
		p->fs.rq_size = user_pipe_64.fs.rq_size;
		p->fs.rq_elements = user_pipe_64.fs.rq_elements;
		p->fs.last_expired = user_pipe_64.fs.last_expired;
		p->fs.backlogged = user_pipe_64.fs.backlogged;
		p->fs.w_q = user_pipe_64.fs.w_q;
		p->fs.max_th = user_pipe_64.fs.max_th;
		p->fs.min_th = user_pipe_64.fs.min_th;
		p->fs.max_p = user_pipe_64.fs.max_p;
		p->fs.c_1 = user_pipe_64.fs.c_1;
		p->fs.c_2 = user_pipe_64.fs.c_2;
		p->fs.c_3 = user_pipe_64.fs.c_3;
		p->fs.c_4 = user_pipe_64.fs.c_4;
		p->fs.lookup_depth = user_pipe_64.fs.lookup_depth;
		p->fs.lookup_step = user_pipe_64.fs.lookup_step;
		p->fs.lookup_weight = user_pipe_64.fs.lookup_weight;
		p->fs.avg_pkt_size = user_pipe_64.fs.avg_pkt_size;
		p->fs.max_pkt_size = user_pipe_64.fs.max_pkt_size;
	}
	return error;
}

static void
cp_flow_set_to_32_user(struct dn_flow_set *set, struct dn_flow_set_32 *fs_bp)
{
	fs_bp->fs_nr = set->fs_nr;
	fs_bp->flags_fs = set->flags_fs;
	fs_bp->parent_nr = set->parent_nr;
	fs_bp->weight = set->weight;
	fs_bp->qsize = set->qsize;
	fs_bp->plr = set->plr;
	fs_bp->flow_mask = set->flow_mask;
	fs_bp->rq_size = set->rq_size;
	fs_bp->rq_elements = set->rq_elements;
	fs_bp->last_expired = set->last_expired;
	fs_bp->backlogged = set->backlogged;
	fs_bp->w_q = set->w_q;
	fs_bp->max_th = set->max_th;
	fs_bp->min_th = set->min_th;
	fs_bp->max_p = set->max_p;
	fs_bp->c_1 = set->c_1;
	fs_bp->c_2 = set->c_2;
	fs_bp->c_3 = set->c_3;
	fs_bp->c_4 = set->c_4;
	fs_bp->w_q_lookup = CAST_DOWN_EXPLICIT(user32_addr_t, set->w_q_lookup);
	fs_bp->lookup_depth = set->lookup_depth;
	fs_bp->lookup_step = set->lookup_step;
	fs_bp->lookup_weight = set->lookup_weight;
	fs_bp->avg_pkt_size = set->avg_pkt_size;
	fs_bp->max_pkt_size = set->max_pkt_size;
}

static void
cp_flow_set_to_64_user(struct dn_flow_set *set, struct dn_flow_set_64 *fs_bp)
{
	fs_bp->fs_nr = set->fs_nr;
	fs_bp->flags_fs = set->flags_fs;
	fs_bp->parent_nr = set->parent_nr;
	fs_bp->weight = set->weight;
	fs_bp->qsize = set->qsize;
	fs_bp->plr = set->plr;
	fs_bp->flow_mask = set->flow_mask;
	fs_bp->rq_size = set->rq_size;
	fs_bp->rq_elements = set->rq_elements;
	fs_bp->last_expired = set->last_expired;
	fs_bp->backlogged = set->backlogged;
	fs_bp->w_q = set->w_q;
	fs_bp->max_th = set->max_th;
	fs_bp->min_th = set->min_th;
	fs_bp->max_p = set->max_p;
	fs_bp->c_1 = set->c_1;
	fs_bp->c_2 = set->c_2;
	fs_bp->c_3 = set->c_3;
	fs_bp->c_4 = set->c_4;
	fs_bp->w_q_lookup = CAST_DOWN(user64_addr_t, set->w_q_lookup);
	fs_bp->lookup_depth = set->lookup_depth;
	fs_bp->lookup_step = set->lookup_step;
	fs_bp->lookup_weight = set->lookup_weight;
	fs_bp->avg_pkt_size = set->avg_pkt_size;
	fs_bp->max_pkt_size = set->max_pkt_size;
}

static
void
cp_queue_to_32_user( struct dn_flow_queue *q, struct dn_flow_queue_32 *qp)
{
	qp->id = q->id;
	qp->len = q->len;
	qp->len_bytes = q->len_bytes;
	qp->numbytes = q->numbytes;
	qp->tot_pkts = q->tot_pkts;
	qp->tot_bytes = q->tot_bytes;
	qp->drops = q->drops;
	qp->hash_slot = q->hash_slot;
	qp->avg = q->avg;
	qp->count = q->count;
	qp->random = q->random;
	qp->q_time = q->q_time;
	qp->heap_pos = q->heap_pos;
	qp->sched_time = q->sched_time;
	qp->S = q->S;
	qp->F = q->F;
}

static
void
cp_queue_to_64_user( struct dn_flow_queue *q, struct dn_flow_queue_64 *qp)
{
	qp->id = q->id;
	qp->len = q->len;
	qp->len_bytes = q->len_bytes;
	qp->numbytes = q->numbytes;
	qp->tot_pkts = q->tot_pkts;
	qp->tot_bytes = q->tot_bytes;
	qp->drops = q->drops;
	qp->hash_slot = q->hash_slot;
	qp->avg = q->avg;
	qp->count = q->count;
	qp->random = q->random;
	qp->q_time = q->q_time;
	qp->heap_pos = q->heap_pos;
	qp->sched_time = q->sched_time;
	qp->S = q->S;
	qp->F = q->F;
}

static
char *
cp_pipe_to_32_user(struct dn_pipe *p, struct dn_pipe_32 *pipe_bp)
{
	char    *bp;

	pipe_bp->pipe_nr = p->pipe_nr;
	pipe_bp->bandwidth = p->bandwidth;
	pipe_bp->delay = p->delay;
	bcopy( &(p->scheduler_heap), &(pipe_bp->scheduler_heap), sizeof(struct dn_heap_32));
	pipe_bp->scheduler_heap.p = CAST_DOWN_EXPLICIT(user32_addr_t, pipe_bp->scheduler_heap.p);
	bcopy( &(p->not_eligible_heap), &(pipe_bp->not_eligible_heap), sizeof(struct dn_heap_32));
	pipe_bp->not_eligible_heap.p = CAST_DOWN_EXPLICIT(user32_addr_t, pipe_bp->not_eligible_heap.p);
	bcopy( &(p->idle_heap), &(pipe_bp->idle_heap), sizeof(struct dn_heap_32));
	pipe_bp->idle_heap.p = CAST_DOWN_EXPLICIT(user32_addr_t, pipe_bp->idle_heap.p);
	pipe_bp->V = p->V;
	pipe_bp->sum = p->sum;
	pipe_bp->numbytes = p->numbytes;
	pipe_bp->sched_time = p->sched_time;
	bcopy( p->if_name, pipe_bp->if_name, IFNAMSIZ);
	pipe_bp->ifp = CAST_DOWN_EXPLICIT(user32_addr_t, p->ifp);
	pipe_bp->ready = p->ready;

	cp_flow_set_to_32_user( &(p->fs), &(pipe_bp->fs));

	pipe_bp->delay = (pipe_bp->delay * 1000) / (hz * 10);
	  
                                                         
                                                       
                                                              
                      
    
	pipe_bp->next = CAST_DOWN_EXPLICIT( user32_addr_t, DN_IS_PIPE );
	                    
	pipe_bp->head = pipe_bp->tail = (user32_addr_t) 0;
	pipe_bp->fs.next = (user32_addr_t)0;
	pipe_bp->fs.pipe = (user32_addr_t)0;
	pipe_bp->fs.rq = (user32_addr_t)0;
	bp = ((char *)pipe_bp) + sizeof(struct dn_pipe_32);
	return dn_copy_set_32( &(p->fs), bp);
}

static
char *
cp_pipe_to_64_user(struct dn_pipe *p, struct dn_pipe_64 *pipe_bp)
{
	char    *bp;

	pipe_bp->pipe_nr = p->pipe_nr;
	pipe_bp->bandwidth = p->bandwidth;
	pipe_bp->delay = p->delay;
	bcopy( &(p->scheduler_heap), &(pipe_bp->scheduler_heap), sizeof(struct dn_heap_64));
	pipe_bp->scheduler_heap.p = CAST_DOWN(user64_addr_t, pipe_bp->scheduler_heap.p);
	bcopy( &(p->not_eligible_heap), &(pipe_bp->not_eligible_heap), sizeof(struct dn_heap_64));
	pipe_bp->not_eligible_heap.p = CAST_DOWN(user64_addr_t, pipe_bp->not_eligible_heap.p);
	bcopy( &(p->idle_heap), &(pipe_bp->idle_heap), sizeof(struct dn_heap_64));
	pipe_bp->idle_heap.p = CAST_DOWN(user64_addr_t, pipe_bp->idle_heap.p);
	pipe_bp->V = p->V;
	pipe_bp->sum = p->sum;
	pipe_bp->numbytes = p->numbytes;
	pipe_bp->sched_time = p->sched_time;
	bcopy( p->if_name, pipe_bp->if_name, IFNAMSIZ);
	pipe_bp->ifp = CAST_DOWN(user64_addr_t, p->ifp);
	pipe_bp->ready = p->ready;

	cp_flow_set_to_64_user( &(p->fs), &(pipe_bp->fs));

	pipe_bp->delay = (pipe_bp->delay * 1000) / (hz * 10);
	  
                                                         
                                                       
                                                              
                      
    
	pipe_bp->next = CAST_DOWN( user64_addr_t, DN_IS_PIPE );
	                    
	pipe_bp->head = pipe_bp->tail = USER_ADDR_NULL;
	pipe_bp->fs.next = USER_ADDR_NULL;
	pipe_bp->fs.pipe = USER_ADDR_NULL;
	pipe_bp->fs.rq = USER_ADDR_NULL;
	bp = ((char *)pipe_bp) + sizeof(struct dn_pipe_64);
	return dn_copy_set_64( &(p->fs), bp);
}

static int
heap_init(struct dn_heap *h, int new_size)
{
	struct dn_heap_entry *p;

	if (h->size >= new_size) {
		printf("dummynet: heap_init, Bogus call, have %d want %d\n",
		    h->size, new_size);
		return 0;
	}
	new_size = (new_size + HEAP_INCREMENT) & ~HEAP_INCREMENT;
	p = _MALLOC(new_size * sizeof(*p), M_DUMMYNET, M_DONTWAIT );
	if (p == NULL) {
		printf("dummynet: heap_init, resize %d failed\n", new_size );
		return 1;            
	}
	if (h->size > 0) {
		bcopy(h->p, p, h->size * sizeof(*p));
		FREE(h->p, M_DUMMYNET);
	}
	h->p = p;
	h->size = new_size;
	return 0;
}

  
                                                              
                                                                  
                                                               
             
                                                        
  
                                                                        
                                                                  
   
#define SET_OFFSET(heap, node) \
    if (heap->offset > 0) \
	    *((int *)((char *)(heap->p[node].object) + heap->offset)) = node ;
  
                                                                              
   
#define RESET_OFFSET(heap, node) \
    if (heap->offset > 0) \
	    *((int *)((char *)(heap->p[node].object) + heap->offset)) = -1 ;
static int
heap_insert(struct dn_heap *h, dn_key key1, void *p)
{
	int son = h->elements;

	if (p == NULL) {                                             
		son = key1;
	} else {                                                            
		son = h->elements;
		if (son == h->size) {                     
			if (heap_init(h, h->elements + 1)) {
				return 1;                 
			}
		}
		h->p[son].object = p;
		h->p[son].key = key1;
		h->elements++;
	}
	while (son > 0) {                                      
		int father = HEAP_FATHER(son);
		struct dn_heap_entry tmp;

		if (DN_KEY_LT( h->p[father].key, h->p[son].key )) {
			break;                           
		}
		                                              
		HEAP_SWAP(h->p[son], h->p[father], tmp);
		SET_OFFSET(h, son);
		son = father;
	}
	SET_OFFSET(h, son);
	return 0;
}

  
                                                      
   
static void
heap_extract(struct dn_heap *h, void *obj)
{
	int child, father, maxelt = h->elements - 1;

	if (maxelt < 0) {
		printf("dummynet: warning, extract from empty heap 0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(h));
		return;
	}
	father = 0;                                      
	if (obj != NULL) {                                                   
		if (h->offset <= 0) {
			panic("dummynet: heap_extract from middle not supported on this heap!!!\n");
		}
		father = *((int *)((char *)obj + h->offset));
		if (father < 0 || father >= h->elements) {
			printf("dummynet: heap_extract, father %d out of bound 0..%d\n",
			    father, h->elements);
			panic("dummynet: heap_extract");
		}
	}
	RESET_OFFSET(h, father);
	child = HEAP_LEFT(father);                      
	while (child <= maxelt) {                        
		if (child != maxelt && DN_KEY_LT(h->p[child + 1].key, h->p[child].key)) {
			child = child + 1;                                       
		}
		h->p[father] = h->p[child];
		SET_OFFSET(h, father);
		father = child;
		child = HEAP_LEFT(child);                               
	}
	h->elements--;
	if (father != maxelt) {
		  
                                                                     
     
		h->p[father] = h->p[maxelt];
		heap_insert(h, father, NULL);                           
	}
}

  
                                                                 
                                                                 
   
static void
heapify(struct dn_heap *h)
{
	int i;

	for (i = 0; i < h->elements; i++) {
		heap_insert(h, i, NULL);
	}
}

  
                                           
   
static void
heap_free(struct dn_heap *h)
{
	if (h->size > 0) {
		FREE(h->p, M_DUMMYNET);
	}
	bzero(h, sizeof(*h));
}

  
                                           
   

  
                                                                      
                                                                      
                                       
   
static struct dn_pkt_tag *
dn_tag_get(struct mbuf *m)
{
	struct m_tag *mtag = m_tag_first(m);

	if (!(mtag != NULL &&
	    mtag->m_tag_id == KERNEL_MODULE_TAG_ID &&
	    mtag->m_tag_type == KERNEL_TAG_TYPE_DUMMYNET)) {
		panic("packet on dummynet queue w/o dummynet tag: 0x%llx",
		    (uint64_t)VM_KERNEL_ADDRPERM(m));
	}

	return (struct dn_pkt_tag *)(mtag + 1);
}

  
                       
  
                                                                
                                                                
                                                                       
                      
  
                                                                       
                                                    
  
                                                                     
                                                   
  
                                                                     
                                                                
                                 
   
static void
transmit_event(struct dn_pipe *pipe, struct mbuf **head, struct mbuf **tail)
{
	struct mbuf *m;
	struct dn_pkt_tag *pkt = NULL;
	u_int64_t schedule_time;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);
	ASSERT(serialize >= 0);
	if (serialize == 0) {
		while ((m = pipe->head) != NULL) {
			pkt = dn_tag_get(m);
			if (!DN_KEY_LEQ(pkt->dn_output_time, curr_time)) {
				break;
			}

			pipe->head = m->m_nextpkt;
			if (*tail != NULL) {
				(*tail)->m_nextpkt = m;
			} else {
				*head = m;
			}
			*tail = m;
		}

		if (*tail != NULL) {
			(*tail)->m_nextpkt = NULL;
		}
	}

	schedule_time = pkt == NULL || DN_KEY_LEQ(pkt->dn_output_time, curr_time) ?
	    curr_time + 1 : pkt->dn_output_time;

	                                                                                    
	if ((m = pipe->head) != NULL) {
		pkt = dn_tag_get(m);
		                                                          
                                                                 
     
		heap_insert(&extract_heap, schedule_time, pipe);
	}
}

  
                                                              
                                                                   
                                                           
   

                                                                 
                                                              
                                                                
                                                                     
                 
   
#define SET_TICKS(_m, q, p)     \
    ((_m)->m_pkthdr.len*8*(hz*10) - (q)->numbytes + p->bandwidth - 1 ) / \
	    p->bandwidth ;

  
                                                             
                                    
   
static void
move_pkt(struct mbuf *pkt, struct dn_flow_queue *q,
    struct dn_pipe *p, int len)
{
	struct dn_pkt_tag *dt = dn_tag_get(pkt);

	q->head = pkt->m_nextpkt;
	q->len--;
	q->len_bytes -= len;

	dt->dn_output_time = curr_time + p->delay;

	if (p->head == NULL) {
		p->head = pkt;
	} else {
		p->tail->m_nextpkt = pkt;
	}
	p->tail = pkt;
	p->tail->m_nextpkt = NULL;
}

  
                                                               
                                                                 
                                      
                                                                      
                                                                   
   
static void
ready_event(struct dn_flow_queue *q, struct mbuf **head, struct mbuf **tail)
{
	struct mbuf *pkt;
	struct dn_pipe *p = q->fs->pipe;
	int p_was_empty;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	if (p == NULL) {
		printf("dummynet: ready_event pipe is gone\n");
		return;
	}
	p_was_empty = (p->head == NULL);

	  
                                                   
                                                              
                                                            
                                                
                                                               
                                        
    
	q->numbytes += (curr_time - q->sched_time) * p->bandwidth;
	while ((pkt = q->head) != NULL) {
		int len = pkt->m_pkthdr.len;
		int len_scaled = p->bandwidth ? len * 8 * (hz * 10) : 0;
		if (len_scaled > q->numbytes) {
			break;
		}
		q->numbytes -= len_scaled;
		move_pkt(pkt, q, p, len);
	}
	  
                                                             
                                                                
                                                  
                                                                   
                                                  
    
	if ((pkt = q->head) != NULL) {                                  
		dn_key t = SET_TICKS(pkt, q, p);                           
		q->sched_time = curr_time;
		heap_insert(&ready_heap, curr_time + t, (void *)q );
		                                                              
                                                    
     
	} else {                                                     
		q->q_time = curr_time;
		q->numbytes = 0;
	}
	  
                                                           
                                                  
    
	if (p_was_empty) {
		transmit_event(p, head, tail);
	}
}

  
                                                                       
                                                                   
                                                        
                                                                 
                                                                   
                                
   
static void
ready_event_wfq(struct dn_pipe *p, struct mbuf **head, struct mbuf **tail)
{
	int p_was_empty = (p->head == NULL);
	struct dn_heap *sch = &(p->scheduler_heap);
	struct dn_heap *neh = &(p->not_eligible_heap);
	int64_t p_numbytes = p->numbytes;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	if (p->if_name[0] == 0) {                            
		p_numbytes += (curr_time - p->sched_time) * p->bandwidth;
	} else {                                                                   
		if (p->ifp && !IFCQ_IS_EMPTY(&p->ifp->if_snd)) {
			return;
		} else {
			DPRINTF(("dummynet: pipe %d ready from %s --\n",
			    p->pipe_nr, p->if_name));
		}
	}

	  
                                                              
                           
    
	while (p_numbytes >= 0 && (sch->elements > 0 || neh->elements > 0)) {
		if (sch->elements > 0) {                                          
			struct dn_flow_queue *q = sch->p[0].object;
			struct mbuf *pkt = q->head;
			struct dn_flow_set *fs = q->fs;
			u_int64_t len = pkt->m_pkthdr.len;
			int len_scaled = p->bandwidth ? len * 8 * (hz * 10) : 0;

			heap_extract(sch, NULL);                             
			p_numbytes -= len_scaled;
			move_pkt(pkt, q, p, len);

			p->V += (len << MY_M) / p->sum;               
			q->S = q->F;                        
			if (q->len == 0) {                                   
				fs->backlogged--;
				heap_insert(&(p->idle_heap), q->F, q);
			} else {                       
				  
                                                      
                                                              
       
				len = (q->head)->m_pkthdr.len;
				q->F += (len << MY_M) / (u_int64_t) fs->weight;
				if (DN_KEY_LEQ(q->S, p->V)) {
					heap_insert(neh, q->S, q);
				} else {
					heap_insert(sch, q->F, q);
				}
			}
		}
		  
                                                                        
                                                                    
                                                                   
                                 
     
		if (sch->elements == 0 && neh->elements > 0) {
			p->V = MAX64( p->V, neh->p[0].key );
		}
		                                                                
		while (neh->elements > 0 && DN_KEY_LEQ(neh->p[0].key, p->V)) {
			struct dn_flow_queue *q = neh->p[0].object;
			heap_extract(neh, NULL);
			heap_insert(sch, q->F, q);
		}

		if (p->if_name[0] != '\0') {                                   
			p_numbytes = -1;                             
			break;
		}
	}
	if (sch->elements == 0 && neh->elements == 0 && p_numbytes >= 0
	    && p->idle_heap.elements > 0) {
		  
                                                                     
     
		int i;

		for (i = 0; i < p->idle_heap.elements; i++) {
			struct dn_flow_queue *q = p->idle_heap.p[i].object;

			q->F = 0;
			q->S = q->F + 1;
		}
		p->sum = 0;
		p->V = 0;
		p->idle_heap.elements = 0;
	}
	  
                                                                     
                                                          
                                                  
    
	if (p->if_name[0] == 0 && p_numbytes < 0) {                                
		dn_key t = 0;                                     

		if (p->bandwidth > 0) {
			t = (p->bandwidth - 1 - p_numbytes) / p->bandwidth;
		}
		dn_tag_get(p->tail)->dn_output_time += t;
		p->sched_time = curr_time;
		heap_insert(&wfq_ready_heap, curr_time + t, (void *)p);
		                                                              
                                                    
     
	}

	                                                                 
	if (p_numbytes > INT_MAX) {
		p->numbytes = INT_MAX;
	} else if (p_numbytes < INT_MIN) {
		p->numbytes = INT_MIN;
	} else {
		p->numbytes = p_numbytes;
	}

	  
                                                           
                                                  
    
	if (p_was_empty) {
		transmit_event(p, head, tail);
	}
}

  
                                          
                                                                  
   
static void
dummynet(__unused void * unused)
{
	void *p;                                   
	struct dn_heap *h;
	struct dn_heap *heaps[3];
	struct mbuf *head = NULL, *tail = NULL;
	int i;
	struct dn_pipe *pe;
	struct timespec ts;
	struct timeval      tv;

	heaps[0] = &ready_heap;                                
	heaps[1] = &wfq_ready_heap;                     
	heaps[2] = &extract_heap;                       

	lck_mtx_lock(dn_mutex);

	                                                    
                                                            
                                             
    
	microuptime(&tv);
	curr_time = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

	for (i = 0; i < 3; i++) {
		h = heaps[i];
		while (h->elements > 0 && DN_KEY_LEQ(h->p[0].key, curr_time)) {
			if (h->p[0].key > curr_time) {
				printf("dummynet: warning, heap %d is %d ticks late\n",
				    i, (int)(curr_time - h->p[0].key));
			}
			p = h->p[0].object;                                       
			heap_extract(h, NULL);                                        
			if (i == 0) {
				ready_event(p, &head, &tail);
			} else if (i == 1) {
				struct dn_pipe *pipe = p;
				if (pipe->if_name[0] != '\0') {
					printf("dummynet: bad ready_event_wfq for pipe %s\n",
					    pipe->if_name);
				} else {
					ready_event_wfq(p, &head, &tail);
				}
			} else {
				transmit_event(p, &head, &tail);
			}
		}
	}
	                                                   
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(pe, &pipehash[i], next) {
			if (pe->idle_heap.elements > 0 &&
			    DN_KEY_LT(pe->idle_heap.p[0].key, pe->V)) {
				struct dn_flow_queue *q = pe->idle_heap.p[0].object;

				heap_extract(&(pe->idle_heap), NULL);
				q->S = q->F + 1;                                
				pe->sum -= q->fs->weight;
			}
		}
	}

	                                                              
                                                      
    
	timer_enabled = 0;
	for (i = 0; i < 3; i++) {
		h = heaps[i];
		if (h->elements > 0) {                 
			ts.tv_sec = 0;
			ts.tv_nsec = 1 * 1000000;             
			timer_enabled = 1;
			bsd_timeout(dummynet, NULL, &ts);
			break;
		}
	}

	if (head != NULL) {
		serialize++;
	}

	lck_mtx_unlock(dn_mutex);

	                                                          
	if (head != NULL) {
		dummynet_send(head);
		lck_mtx_lock(dn_mutex);
		serialize--;
		lck_mtx_unlock(dn_mutex);
	}
}


static void
dummynet_send(struct mbuf *m)
{
	struct dn_pkt_tag *pkt;
	struct mbuf *n;

	for (; m != NULL; m = n) {
		n = m->m_nextpkt;
		m->m_nextpkt = NULL;
		pkt = dn_tag_get(m);

		DPRINTF(("dummynet_send m: 0x%llx dn_dir: %d dn_flags: 0x%x\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(m), pkt->dn_dir,
		    pkt->dn_flags));

		switch (pkt->dn_dir) {
		case DN_TO_IP_OUT: {
			struct route tmp_rt;

			                                            
			bzero(&tmp_rt, sizeof(tmp_rt));

			                                                         
			pkt->dn_flags |= IP_RAWOUTPUT | IP_FORWARDING;
			(void)ip_output(m, NULL, &tmp_rt, pkt->dn_flags, NULL, NULL);
			ROUTE_RELEASE(&tmp_rt);
			break;
		}
		case DN_TO_IP_IN:
			proto_inject(PF_INET, m);
			break;
		case DN_TO_IP6_OUT: {
			                                                  
			ip6_output(m, NULL, NULL, IPV6_FORWARDING, NULL, NULL, NULL);
			break;
		}
		case DN_TO_IP6_IN:
			proto_inject(PF_INET6, m);
			break;
		default:
			printf("dummynet: bad switch %d!\n", pkt->dn_dir);
			m_freem(m);
			break;
		}
	}
}

  
                                                           
                                      
   
static int
expire_queues(struct dn_flow_set *fs)
{
	struct dn_flow_queue *q, *prev;
	int i, initial_elements = fs->rq_elements;
	struct timeval timenow;

	                                     
	getmicrotime(&timenow);

	if (fs->last_expired == timenow.tv_sec) {
		return 0;
	}
	fs->last_expired = timenow.tv_sec;
	for (i = 0; i <= fs->rq_size; i++) {                           
		for (prev = NULL, q = fs->rq[i]; q != NULL;) {
			if (q->head != NULL || q->S != q->F + 1) {
				prev = q;
				q = q->next;
			} else {                               
				struct dn_flow_queue *old_q = q;

				if (prev != NULL) {
					prev->next = q = q->next;
				} else {
					fs->rq[i] = q = q->next;
				}
				fs->rq_elements--;
				FREE(old_q, M_DUMMYNET);
			}
		}
	}
	return initial_elements - fs->rq_elements;
}

  
                                                         
                                              
   
static struct dn_flow_queue *
create_queue(struct dn_flow_set *fs, int i)
{
	struct dn_flow_queue *q;

	if (fs->rq_elements > fs->rq_size * dn_max_ratio &&
	    expire_queues(fs) == 0) {
		  
                                                      
     
		i = fs->rq_size;
		if (fs->rq[i] != NULL) {
			return fs->rq[i];
		}
	}
	q = _MALLOC(sizeof(*q), M_DUMMYNET, M_DONTWAIT | M_ZERO);
	if (q == NULL) {
		printf("dummynet: sorry, cannot allocate queue for new flow\n");
		return NULL;
	}
	q->fs = fs;
	q->hash_slot = i;
	q->next = fs->rq[i];
	q->S = q->F + 1;                                       
	fs->rq[i] = q;
	fs->rq_elements++;
	return q;
}

  
                                                                
                                                         
                                           
   
static struct dn_flow_queue *
find_queue(struct dn_flow_set *fs, struct ip_flow_id *id)
{
	int i = 0;                                          
	struct dn_flow_queue *q, *prev;
	int is_v6 = IS_IP6_FLOW_ID(id);

	if (!(fs->flags_fs & DN_HAVE_FLOW_MASK)) {
		q = fs->rq[0];
	} else {
		                                      
		id->dst_port &= fs->flow_mask.dst_port;
		id->src_port &= fs->flow_mask.src_port;
		id->proto &= fs->flow_mask.proto;
		id->flags = 0;                                   
		if (is_v6) {
			APPLY_MASK(&id->dst_ip6, &fs->flow_mask.dst_ip6);
			APPLY_MASK(&id->src_ip6, &fs->flow_mask.src_ip6);
			id->flow_id6 &= fs->flow_mask.flow_id6;

			i = ((id->dst_ip6.__u6_addr.__u6_addr32[0]) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[1]) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[2]) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[3]) & 0xffff) ^

			    ((id->dst_ip6.__u6_addr.__u6_addr32[0] >> 15) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[1] >> 15) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[2] >> 15) & 0xffff) ^
			    ((id->dst_ip6.__u6_addr.__u6_addr32[3] >> 15) & 0xffff) ^

			    ((id->src_ip6.__u6_addr.__u6_addr32[0] << 1) & 0xfffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[1] << 1) & 0xfffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[2] << 1) & 0xfffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[3] << 1) & 0xfffff) ^

			    ((id->src_ip6.__u6_addr.__u6_addr32[0] >> 16) & 0xffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[1] >> 16) & 0xffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[2] >> 16) & 0xffff) ^
			    ((id->src_ip6.__u6_addr.__u6_addr32[3] >> 16) & 0xffff) ^

			    (id->dst_port << 1) ^ (id->src_port) ^
			    (id->proto) ^
			    (id->flow_id6);
		} else {
			id->dst_ip &= fs->flow_mask.dst_ip;
			id->src_ip &= fs->flow_mask.src_ip;

			i = ((id->dst_ip) & 0xffff) ^
			    ((id->dst_ip >> 15) & 0xffff) ^
			    ((id->src_ip << 1) & 0xffff) ^
			    ((id->src_ip >> 16) & 0xffff) ^
			    (id->dst_port << 1) ^ (id->src_port) ^
			    (id->proto);
		}
		i = i % fs->rq_size;
		                                                
		searches++;
		for (prev = NULL, q = fs->rq[i]; q;) {
			search_steps++;
			if (is_v6 &&
			    IN6_ARE_ADDR_EQUAL(&id->dst_ip6, &q->id.dst_ip6) &&
			    IN6_ARE_ADDR_EQUAL(&id->src_ip6, &q->id.src_ip6) &&
			    id->dst_port == q->id.dst_port &&
			    id->src_port == q->id.src_port &&
			    id->proto == q->id.proto &&
			    id->flags == q->id.flags &&
			    id->flow_id6 == q->id.flow_id6) {
				break;            
			}
			if (!is_v6 && id->dst_ip == q->id.dst_ip &&
			    id->src_ip == q->id.src_ip &&
			    id->dst_port == q->id.dst_port &&
			    id->src_port == q->id.src_port &&
			    id->proto == q->id.proto &&
			    id->flags == q->id.flags) {
				break;            
			}
			                                                
			if (pipe_expire && q->head == NULL && q->S == q->F + 1) {
				                                                  
				struct dn_flow_queue *old_q = q;

				if (prev != NULL) {
					prev->next = q = q->next;
				} else {
					fs->rq[i] = q = q->next;
				}
				fs->rq_elements--;
				FREE(old_q, M_DUMMYNET);
				continue;
			}
			prev = q;
			q = q->next;
		}
		if (q && prev != NULL) {                             
			prev->next = q->next;
			q->next = fs->rq[i];
			fs->rq[i] = q;
		}
	}
	if (q == NULL) {                                             
		q = create_queue(fs, i);
		if (q != NULL) {
			q->id = *id;
		}
	}
	return q;
}

static int
red_drops(struct dn_flow_set *fs, struct dn_flow_queue *q, int len)
{
	  
                 
   
                                                                       
                                                      
                                              
                                                                      
   
                                                                  
                            
                                                                      
   
                                                
                                                                   
                                               
   
    

	int64_t p_b = 0;
	                                 
	u_int q_size = (fs->flags_fs & DN_QSIZE_IS_BYTES) ? q->len_bytes : q->len;

	DPRINTF(("\ndummynet: %d q: %2u ", (int) curr_time, q_size));

	                                   
	if (q_size != 0) {
		  
                                                          
     
		int diff = SCALE(q_size) - q->avg;
		int64_t v = SCALE_MUL((int64_t) diff, (int64_t) fs->w_q);

		q->avg += (int) v;
	} else {
		  
                                                         
                                               
                                                            
                    
                       
     
		if (q->avg) {
			u_int t = (curr_time - q->q_time) / fs->lookup_step;

			q->avg = (t < fs->lookup_depth) ?
			    SCALE_MUL(q->avg, fs->w_q_lookup[t]) : 0;
		}
	}
	DPRINTF(("dummynet: avg: %u ", SCALE_VAL(q->avg)));

	                     

	if (q->avg < fs->min_th) {
		q->count = -1;
		return 0;                      
	}
	if (q->avg >= fs->max_th) {                                      
		if (fs->flags_fs & DN_IS_GENTLE_RED) {
			  
                                                                
                                          
                           
                                                               
      
			p_b = SCALE_MUL((int64_t) fs->c_3, (int64_t) q->avg) - fs->c_4;
		} else {
			q->count = -1;
			DPRINTF(("dummynet: - drop"));
			return 1;
		}
	} else if (q->avg > fs->min_th) {
		  
                                                                  
                                                                
                                       
     
		p_b = SCALE_MUL((int64_t) fs->c_1, (int64_t) q->avg) - fs->c_2;
	}
	if (fs->flags_fs & DN_QSIZE_IS_BYTES) {
		p_b = (p_b * len) / fs->max_pkt_size;
	}
	if (++q->count == 0) {
		q->random = (my_random() & 0xffff);
	} else {
		  
                                                                  
                                                               
     
		if (SCALE_MUL(p_b, SCALE((int64_t) q->count)) > q->random) {
			q->count = 0;
			DPRINTF(("dummynet: - red drop"));
			                                                  
			q->random = (my_random() & 0xffff);
			return 1;           
		}
	}
	                          
	return 0;             
}

static __inline
struct dn_flow_set *
locate_flowset(int fs_nr)
{
	struct dn_flow_set *fs;
	SLIST_FOREACH(fs, &flowsethash[HASH(fs_nr)], next) {
		if (fs->fs_nr == fs_nr) {
			return fs;
		}
	}

	return NULL;
}

static __inline struct dn_pipe *
locate_pipe(int pipe_nr)
{
	struct dn_pipe *pipe;

	SLIST_FOREACH(pipe, &pipehash[HASH(pipe_nr)], next) {
		if (pipe->pipe_nr == pipe_nr) {
			return pipe;
		}
	}

	return NULL;
}



  
                                                               
                                                 
  
                                                    
                                                      
                              
                                            
                                                         
                           
                                                               
                                                   
                                                  
                                                      
  
   
static int
dummynet_io(struct mbuf *m, int pipe_nr, int dir, struct ip_fw_args *fwa)
{
	struct mbuf *head = NULL, *tail = NULL;
	struct dn_pkt_tag *pkt;
	struct m_tag *mtag;
	struct dn_flow_set *fs = NULL;
	struct dn_pipe *pipe;
	u_int64_t len = m->m_pkthdr.len;
	struct dn_flow_queue *q = NULL;
	int is_pipe = 0;
	struct timespec ts;
	struct timeval      tv;

	DPRINTF(("dummynet_io m: 0x%llx pipe: %d dir: %d\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(m), pipe_nr, dir));


#if DUMMYNET
	is_pipe = fwa->fwa_flags == DN_IS_PIPE ? 1 : 0;
#endif               

	pipe_nr &= 0xffff;

	lck_mtx_lock(dn_mutex);

	                                                    
                                                            
                                             
    
	microuptime(&tv);
	curr_time = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

	  
                                                                    
    
	if (is_pipe) {
		pipe = locate_pipe(pipe_nr);
		if (pipe != NULL) {
			fs = &(pipe->fs);
		}
	} else {
		fs = locate_flowset(pipe_nr);
	}


	if (fs == NULL) {
		goto dropit;                                      
	}
	pipe = fs->pipe;
	if (pipe == NULL) {                                                
		pipe = locate_pipe(fs->parent_nr);

		if (pipe != NULL) {
			fs->pipe = pipe;
		} else {
			printf("dummynet: no pipe %d for queue %d, drop pkt\n",
			    fs->parent_nr, fs->fs_nr);
			goto dropit;
		}
	}
	q = find_queue(fs, &(fwa->fwa_id));
	if (q == NULL) {
		goto dropit;                                
	}
	  
                                                     
    
	q->tot_bytes += len;
	q->tot_pkts++;
	if (fs->plr && (my_random() < fs->plr)) {
		goto dropit;                           
	}
	if (fs->flags_fs & DN_QSIZE_IS_BYTES) {
		if (q->len_bytes > fs->qsize) {
			goto dropit;                            
		}
	} else {
		if (q->len >= fs->qsize) {
			goto dropit;                             
		}
	}
	if (fs->flags_fs & DN_IS_RED && red_drops(fs, q, len)) {
		goto dropit;
	}

	                                                   
	mtag = m_tag_create(KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_DUMMYNET,
	    sizeof(struct dn_pkt_tag), M_NOWAIT, m);
	if (mtag == NULL) {
		goto dropit;                                               
	}
	m_tag_prepend(m, mtag);                           

	pkt = (struct dn_pkt_tag *)(mtag + 1);
	bzero(pkt, sizeof(struct dn_pkt_tag));
	                                     
	                                           
	pkt->dn_pf_rule = fwa->fwa_pf_rule;
	pkt->dn_dir = dir;

	pkt->dn_ifp = fwa->fwa_oif;
	if (dir == DN_TO_IP_OUT) {
		  
                                                                 
                                                                  
                                                  
     
		if (fwa->fwa_ro) {
			route_copyout(&pkt->dn_ro, fwa->fwa_ro, sizeof(pkt->dn_ro));
		}
		if (fwa->fwa_dst) {
			if (fwa->fwa_dst == (struct sockaddr_in *)&fwa->fwa_ro->ro_dst) {                         
				fwa->fwa_dst = (struct sockaddr_in *)&(pkt->dn_ro.ro_dst);
			}

			bcopy(fwa->fwa_dst, &pkt->dn_dst, sizeof(pkt->dn_dst));
		}
	} else if (dir == DN_TO_IP6_OUT) {
		if (fwa->fwa_ro6) {
			route_copyout((struct route *)&pkt->dn_ro6,
			    (struct route *)fwa->fwa_ro6, sizeof(pkt->dn_ro6));
		}
		if (fwa->fwa_ro6_pmtu) {
			route_copyout((struct route *)&pkt->dn_ro6_pmtu,
			    (struct route *)fwa->fwa_ro6_pmtu, sizeof(pkt->dn_ro6_pmtu));
		}
		if (fwa->fwa_dst6) {
			if (fwa->fwa_dst6 == (struct sockaddr_in6 *)&fwa->fwa_ro6->ro_dst) {                         
				fwa->fwa_dst6 = (struct sockaddr_in6 *)&(pkt->dn_ro6.ro_dst);
			}

			bcopy(fwa->fwa_dst6, &pkt->dn_dst6, sizeof(pkt->dn_dst6));
		}
		pkt->dn_origifp = fwa->fwa_origifp;
		pkt->dn_mtu = fwa->fwa_mtu;
		pkt->dn_unfragpartlen = fwa->fwa_unfragpartlen;
		if (fwa->fwa_exthdrs) {
			bcopy(fwa->fwa_exthdrs, &pkt->dn_exthdrs, sizeof(pkt->dn_exthdrs));
			  
                                                        
                                    
      
			bzero(fwa->fwa_exthdrs, sizeof(struct ip6_exthdrs));
		}
	}
	if (dir == DN_TO_IP_OUT || dir == DN_TO_IP6_OUT) {
		pkt->dn_flags = fwa->fwa_oflags;
		if (fwa->fwa_ipoa != NULL) {
			pkt->dn_ipoa = *(fwa->fwa_ipoa);
		}
	}
	if (q->head == NULL) {
		q->head = m;
	} else {
		q->tail->m_nextpkt = m;
	}
	q->tail = m;
	q->len++;
	q->len_bytes += len;

	if (q->head != m) {                                         
		goto done;
	}
	  
                                                                   
                                                                     
                
    
	if (is_pipe) {
		  
                                                       
     
		dn_key t = 0;
		if (pipe->bandwidth) {
			t = SET_TICKS(m, q, pipe);
		}
		q->sched_time = curr_time;
		if (t == 0) {                          
			ready_event( q, &head, &tail );
		} else {
			heap_insert(&ready_heap, curr_time + t, q );
		}
	} else {
		  
                                                                    
                                                                     
                                                                 
                                     
                                                    
                                                 
                                            
     
		if (DN_KEY_GT(q->S, q->F)) {                                   
			q->S = pipe->V;
			pipe->sum += fs->weight;                              
		} else {
			heap_extract(&(pipe->idle_heap), q);
			q->S = MAX64(q->F, pipe->V );
		}
		q->F = q->S + (len << MY_M) / (u_int64_t) fs->weight;

		if (pipe->not_eligible_heap.elements == 0 &&
		    pipe->scheduler_heap.elements == 0) {
			pipe->V = MAX64( q->S, pipe->V );
		}
		fs->backlogged++;
		  
                                                              
                                                                 
                                                                 
                                                                
                                                                 
                                                                
                     
                                                               
                                                            
                                                                       
                                   
     
		if (DN_KEY_GT(q->S, pipe->V)) {                   
			if (pipe->scheduler_heap.elements == 0) {
				printf("dummynet: ++ ouch! not eligible but empty scheduler!\n");
			}
			heap_insert(&(pipe->not_eligible_heap), q->S, q);
		} else {
			heap_insert(&(pipe->scheduler_heap), q->F, q);
			if (pipe->numbytes >= 0) {                   
				if (pipe->scheduler_heap.elements != 1) {
					printf("dummynet: OUCH! pipe should have been idle!\n");
				}
				DPRINTF(("dummynet: waking up pipe %d at %d\n",
				    pipe->pipe_nr, (int)(q->F >> MY_M)));
				pipe->sched_time = curr_time;
				ready_event_wfq(pipe, &head, &tail);
			}
		}
	}
done:
	                                                       
	if (!timer_enabled) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1 * 1000000;             
		timer_enabled = 1;
		bsd_timeout(dummynet, NULL, &ts);
	}

	lck_mtx_unlock(dn_mutex);

	if (head != NULL) {
		dummynet_send(head);
	}

	return 0;

dropit:
	if (q) {
		q->drops++;
	}
	lck_mtx_unlock(dn_mutex);
	m_freem(m);
	return (fs && (fs->flags_fs & DN_NOERROR)) ? 0 : ENOBUFS;
}

  
                                                                             
                                                                
   
#define DN_FREE_PKT(_m) do {                                    \
	struct m_tag *tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_DUMMYNET, NULL); \
	if (tag) {                                              \
	        struct dn_pkt_tag *n = (struct dn_pkt_tag *)(tag+1);    \
	        ROUTE_RELEASE(&n->dn_ro);                       \
	}                                                       \
	m_tag_delete(_m, tag);                                  \
	m_freem(_m);                                            \
} while (0)

  
                                                     
                                                            
                                   
                                                         
   
static void
purge_flow_set(struct dn_flow_set *fs, int all)
{
	struct dn_flow_queue *q, *qn;
	int i;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	for (i = 0; i <= fs->rq_size; i++) {
		for (q = fs->rq[i]; q; q = qn) {
			struct mbuf *m, *mnext;

			mnext = q->head;
			while ((m = mnext) != NULL) {
				mnext = m->m_nextpkt;
				DN_FREE_PKT(m);
			}
			qn = q->next;
			FREE(q, M_DUMMYNET);
		}
		fs->rq[i] = NULL;
	}
	fs->rq_elements = 0;
	if (all) {
		                             
		if (fs->w_q_lookup) {
			FREE(fs->w_q_lookup, M_DUMMYNET);
		}
		if (fs->rq) {
			FREE(fs->rq, M_DUMMYNET);
		}
		                                               
		if (fs->pipe && fs != &(fs->pipe->fs)) {
			FREE(fs, M_DUMMYNET);
		}
	}
}

  
                                                         
                                                               
                 
   
static void
purge_pipe(struct dn_pipe *pipe)
{
	struct mbuf *m, *mnext;

	purge_flow_set( &(pipe->fs), 1 );

	mnext = pipe->head;
	while ((m = mnext) != NULL) {
		mnext = m->m_nextpkt;
		DN_FREE_PKT(m);
	}

	heap_free( &(pipe->scheduler_heap));
	heap_free( &(pipe->not_eligible_heap));
	heap_free( &(pipe->idle_heap));
}

  
                                               
   
static void
dummynet_flush(void)
{
	struct dn_pipe *pipe, *pipe1;
	struct dn_flow_set *fs, *fs1;
	int i;

	lck_mtx_lock(dn_mutex);


	                                                  
	heap_free(&ready_heap);
	heap_free(&wfq_ready_heap);
	heap_free(&extract_heap);

	  
                                                   
   
                                                           
    
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH_SAFE(fs, &flowsethash[i], next, fs1) {
			SLIST_REMOVE(&flowsethash[i], fs, dn_flow_set, next);
			purge_flow_set(fs, 1);
		}
	}
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH_SAFE(pipe, &pipehash[i], next, pipe1) {
			SLIST_REMOVE(&pipehash[i], pipe, dn_pipe, next);
			purge_pipe(pipe);
			FREE(pipe, M_DUMMYNET);
		}
	}
	lck_mtx_unlock(dn_mutex);
}

  
                       
   
static int
config_red(struct dn_flow_set *p, struct dn_flow_set * x)
{
	int i;

	x->w_q = p->w_q;
	x->min_th = SCALE(p->min_th);
	x->max_th = SCALE(p->max_th);
	x->max_p = p->max_p;

	x->c_1 = p->max_p / (p->max_th - p->min_th);
	x->c_2 = SCALE_MUL(x->c_1, SCALE(p->min_th));
	if (x->flags_fs & DN_IS_GENTLE_RED) {
		x->c_3 = (SCALE(1) - p->max_p) / p->max_th;
		x->c_4 = (SCALE(1) - 2 * p->max_p);
	}

	                                                                 
	if (x->w_q_lookup) {
		FREE(x->w_q_lookup, M_DUMMYNET);
		x->w_q_lookup = NULL;
	}
	if (red_lookup_depth == 0) {
		printf("\ndummynet: net.inet.ip.dummynet.red_lookup_depth must be > 0\n");
		FREE(x, M_DUMMYNET);
		return EINVAL;
	}
	x->lookup_depth = red_lookup_depth;
	x->w_q_lookup = (u_int *) _MALLOC(x->lookup_depth * sizeof(int),
	    M_DUMMYNET, M_DONTWAIT);
	if (x->w_q_lookup == NULL) {
		printf("dummynet: sorry, cannot allocate red lookup table\n");
		FREE(x, M_DUMMYNET);
		return ENOSPC;
	}

	                                            
	x->lookup_step = p->lookup_step;
	x->lookup_weight = p->lookup_weight;
	x->w_q_lookup[0] = SCALE(1) - x->w_q;
	for (i = 1; i < x->lookup_depth; i++) {
		x->w_q_lookup[i] = SCALE_MUL(x->w_q_lookup[i - 1], x->lookup_weight);
	}
	if (red_avg_pkt_size < 1) {
		red_avg_pkt_size = 512;
	}
	x->avg_pkt_size = red_avg_pkt_size;
	if (red_max_pkt_size < 1) {
		red_max_pkt_size = 1500;
	}
	x->max_pkt_size = red_max_pkt_size;
	return 0;
}

static int
alloc_hash(struct dn_flow_set *x, struct dn_flow_set *pfs)
{
	if (x->flags_fs & DN_HAVE_FLOW_MASK) {                          
		int l = pfs->rq_size;

		if (l == 0) {
			l = dn_hash_size;
		}
		if (l < 4) {
			l = 4;
		} else if (l > DN_MAX_HASH_SIZE) {
			l = DN_MAX_HASH_SIZE;
		}
		x->rq_size = l;
	} else {                                             
		x->rq_size = 1;
	}
	x->rq = _MALLOC((1 + x->rq_size) * sizeof(struct dn_flow_queue *),
	    M_DUMMYNET, M_DONTWAIT | M_ZERO);
	if (x->rq == NULL) {
		printf("dummynet: sorry, cannot allocate queue\n");
		return ENOSPC;
	}
	x->rq_elements = 0;
	return 0;
}

static void
set_fs_parms(struct dn_flow_set *x, struct dn_flow_set *src)
{
	x->flags_fs = src->flags_fs;
	x->qsize = src->qsize;
	x->plr = src->plr;
	x->flow_mask = src->flow_mask;
	if (x->flags_fs & DN_QSIZE_IS_BYTES) {
		if (x->qsize > 1024 * 1024) {
			x->qsize = 1024 * 1024;
		}
	} else {
		if (x->qsize == 0) {
			x->qsize = 50;
		}
		if (x->qsize > 100) {
			x->qsize = 50;
		}
	}
	                     
	if (x->flags_fs & DN_IS_RED) {
		config_red(src, x);                              
	}
}

  
                                  
   
static int
config_pipe(struct dn_pipe *p)
{
	int i, r;
	struct dn_flow_set *pfs = &(p->fs);
	struct dn_flow_queue *q;

	  
                                                    
                                         
                                              
                       
    
	p->delay = (p->delay * (hz * 10)) / 1000;
	                                                       
	if (p->pipe_nr == 0 && pfs->fs_nr == 0) {
		return EINVAL;
	}
	if (p->pipe_nr != 0 && pfs->fs_nr != 0) {
		return EINVAL;
	}
	if (p->pipe_nr != 0) {                     
		struct dn_pipe *x, *b;
		struct dummynet_event dn_event;
		lck_mtx_lock(dn_mutex);

		                 
		b = locate_pipe(p->pipe_nr);

		if (b == NULL || b->pipe_nr != p->pipe_nr) {               
			x = _MALLOC(sizeof(struct dn_pipe), M_DUMMYNET, M_DONTWAIT | M_ZERO);
			if (x == NULL) {
				lck_mtx_unlock(dn_mutex);
				printf("dummynet: no memory for new pipe\n");
				return ENOSPC;
			}
			x->pipe_nr = p->pipe_nr;
			x->fs.pipe = x;
			                                                                   
      
			x->idle_heap.size = x->idle_heap.elements = 0;
			x->idle_heap.offset = offsetof(struct dn_flow_queue, heap_pos);
		} else {
			x = b;
			                                             
			for (i = 0; i <= x->fs.rq_size; i++) {
				for (q = x->fs.rq[i]; q; q = q->next) {
					q->numbytes = 0;
				}
			}
		}

		x->bandwidth = p->bandwidth;
		x->numbytes = 0;                      
		bcopy(p->if_name, x->if_name, sizeof(p->if_name));
		x->ifp = NULL;                          
		x->delay = p->delay;
		set_fs_parms(&(x->fs), pfs);


		if (x->fs.rq == NULL) {                 
			r = alloc_hash(&(x->fs), pfs);
			if (r) {
				lck_mtx_unlock(dn_mutex);
				FREE(x, M_DUMMYNET);
				return r;
			}
			SLIST_INSERT_HEAD(&pipehash[HASH(x->pipe_nr)],
			    x, next);
		}
		lck_mtx_unlock(dn_mutex);

		bzero(&dn_event, sizeof(dn_event));
		dn_event.dn_event_code = DUMMYNET_PIPE_CONFIG;
		dn_event.dn_event_pipe_config.bandwidth = p->bandwidth;
		dn_event.dn_event_pipe_config.delay = p->delay;
		dn_event.dn_event_pipe_config.plr = pfs->plr;

		dummynet_event_enqueue_nwk_wq_entry(&dn_event);
	} else {                   
		struct dn_flow_set *x, *b;

		lck_mtx_lock(dn_mutex);
		                     
		b = locate_flowset(pfs->fs_nr);

		if (b == NULL || b->fs_nr != pfs->fs_nr) {           
			if (pfs->parent_nr == 0) {                          
				lck_mtx_unlock(dn_mutex);
				return EINVAL;
			}
			x = _MALLOC(sizeof(struct dn_flow_set), M_DUMMYNET, M_DONTWAIT | M_ZERO);
			if (x == NULL) {
				lck_mtx_unlock(dn_mutex);
				printf("dummynet: no memory for new flow_set\n");
				return ENOSPC;
			}
			x->fs_nr = pfs->fs_nr;
			x->parent_nr = pfs->parent_nr;
			x->weight = pfs->weight;
			if (x->weight == 0) {
				x->weight = 1;
			} else if (x->weight > 100) {
				x->weight = 100;
			}
		} else {
			                                                              
			if (pfs->parent_nr != 0 && b->parent_nr != pfs->parent_nr) {
				lck_mtx_unlock(dn_mutex);
				return EINVAL;
			}
			x = b;
		}
		set_fs_parms(x, pfs);

		if (x->rq == NULL) {                     
			r = alloc_hash(x, pfs);
			if (r) {
				lck_mtx_unlock(dn_mutex);
				FREE(x, M_DUMMYNET);
				return r;
			}
			SLIST_INSERT_HEAD(&flowsethash[HASH(x->fs_nr)],
			    x, next);
		}
		lck_mtx_unlock(dn_mutex);
	}
	return 0;
}

  
                                                                   
                                  
   
static void
fs_remove_from_heap(struct dn_heap *h, struct dn_flow_set *fs)
{
	int i = 0, found = 0;
	for (; i < h->elements;) {
		if (((struct dn_flow_queue *)h->p[i].object)->fs == fs) {
			h->elements--;
			h->p[i] = h->p[h->elements];
			found++;
		} else {
			i++;
		}
	}
	if (found) {
		heapify(h);
	}
}

  
                                                                           
   
static void
pipe_remove_from_heap(struct dn_heap *h, struct dn_pipe *p)
{
	if (h->elements > 0) {
		int i = 0;
		for (i = 0; i < h->elements; i++) {
			if (h->p[i].object == p) {               
				h->elements--;
				h->p[i] = h->p[h->elements];
				heapify(h);
				break;
			}
		}
	}
}

  
                                                            
   
void
dummynet_drain(void)
{
	struct dn_flow_set *fs;
	struct dn_pipe *p;
	struct mbuf *m, *mnext;
	int i;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	heap_free(&ready_heap);
	heap_free(&wfq_ready_heap);
	heap_free(&extract_heap);
	                                                       
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(fs, &flowsethash[i], next) {
			purge_flow_set(fs, 0);
		}
	}

	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(p, &pipehash[i], next) {
			purge_flow_set(&(p->fs), 0);

			mnext = p->head;
			while ((m = mnext) != NULL) {
				mnext = m->m_nextpkt;
				DN_FREE_PKT(m);
			}
			p->head = p->tail = NULL;
		}
	}
}

  
                                                               
   
static int
delete_pipe(struct dn_pipe *p)
{
	if (p->pipe_nr == 0 && p->fs.fs_nr == 0) {
		return EINVAL;
	}
	if (p->pipe_nr != 0 && p->fs.fs_nr != 0) {
		return EINVAL;
	}
	if (p->pipe_nr != 0) {                                
		struct dn_pipe *b;
		struct dn_flow_set *fs;
		int i;

		lck_mtx_lock(dn_mutex);
		                 
		b = locate_pipe(p->pipe_nr);
		if (b == NULL) {
			lck_mtx_unlock(dn_mutex);
			return EINVAL;                
		}

		                                
		SLIST_REMOVE(&pipehash[HASH(b->pipe_nr)], b, dn_pipe, next);


		                                                        
		for (i = 0; i < HASHSIZE; i++) {
			SLIST_FOREACH(fs, &flowsethash[i], next) {
				if (fs->pipe == b) {
					printf("dummynet: ++ ref to pipe %d from fs %d\n",
					    p->pipe_nr, fs->fs_nr);
					fs->pipe = NULL;
					purge_flow_set(fs, 0);
				}
			}
		}
		fs_remove_from_heap(&ready_heap, &(b->fs));

		purge_pipe(b);                                              
		                                                                   
		pipe_remove_from_heap(&extract_heap, b);
		pipe_remove_from_heap(&wfq_ready_heap, b);
		lck_mtx_unlock(dn_mutex);

		FREE(b, M_DUMMYNET);
	} else {                                         
		struct dn_flow_set *b;

		lck_mtx_lock(dn_mutex);
		                
		b = locate_flowset(p->fs.fs_nr);
		if (b == NULL) {
			lck_mtx_unlock(dn_mutex);
			return EINVAL;                
		}


		                                   
		SLIST_REMOVE( &flowsethash[HASH(b->fs_nr)], b, dn_flow_set, next);

		if (b->pipe != NULL) {
			                                                                 
			b->pipe->sum -= b->weight * b->backlogged;
			fs_remove_from_heap(&(b->pipe->not_eligible_heap), b);
			fs_remove_from_heap(&(b->pipe->scheduler_heap), b);
#if 1                                                     
			fs_remove_from_heap(&(b->pipe->idle_heap), b);
#endif
		}
		purge_flow_set(b, 1);
		lck_mtx_unlock(dn_mutex);
	}
	return 0;
}

  
                                                                
   
static
char*
dn_copy_set_32(struct dn_flow_set *set, char *bp)
{
	int i, copied = 0;
	struct dn_flow_queue *q;
	struct dn_flow_queue_32 *qp = (struct dn_flow_queue_32 *)bp;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	for (i = 0; i <= set->rq_size; i++) {
		for (q = set->rq[i]; q; q = q->next, qp++) {
			if (q->hash_slot != i) {
				printf("dummynet: ++ at %d: wrong slot (have %d, "
				    "should be %d)\n", copied, q->hash_slot, i);
			}
			if (q->fs != set) {
				printf("dummynet: ++ at %d: wrong fs ptr "
				    "(have 0x%llx, should be 0x%llx)\n", i,
				    (uint64_t)VM_KERNEL_ADDRPERM(q->fs),
				    (uint64_t)VM_KERNEL_ADDRPERM(set));
			}
			copied++;
			cp_queue_to_32_user( q, qp );
			                      
			qp->next = (user32_addr_t)0;
			qp->head = qp->tail = (user32_addr_t)0;
			qp->fs = (user32_addr_t)0;
		}
	}
	if (copied != set->rq_elements) {
		printf("dummynet: ++ wrong count, have %d should be %d\n",
		    copied, set->rq_elements);
	}
	return (char *)qp;
}

static
char*
dn_copy_set_64(struct dn_flow_set *set, char *bp)
{
	int i, copied = 0;
	struct dn_flow_queue *q;
	struct dn_flow_queue_64 *qp = (struct dn_flow_queue_64 *)bp;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);

	for (i = 0; i <= set->rq_size; i++) {
		for (q = set->rq[i]; q; q = q->next, qp++) {
			if (q->hash_slot != i) {
				printf("dummynet: ++ at %d: wrong slot (have %d, "
				    "should be %d)\n", copied, q->hash_slot, i);
			}
			if (q->fs != set) {
				printf("dummynet: ++ at %d: wrong fs ptr "
				    "(have 0x%llx, should be 0x%llx)\n", i,
				    (uint64_t)VM_KERNEL_ADDRPERM(q->fs),
				    (uint64_t)VM_KERNEL_ADDRPERM(set));
			}
			copied++;
			                           
			cp_queue_to_64_user( q, qp );
			                      
			qp->next = USER_ADDR_NULL;
			qp->head = qp->tail = USER_ADDR_NULL;
			qp->fs = USER_ADDR_NULL;
		}
	}
	if (copied != set->rq_elements) {
		printf("dummynet: ++ wrong count, have %d should be %d\n",
		    copied, set->rq_elements);
	}
	return (char *)qp;
}

static size_t
dn_calc_size(int is64user)
{
	struct dn_flow_set *set;
	struct dn_pipe *p;
	size_t size = 0;
	size_t pipesize;
	size_t queuesize;
	size_t setsize;
	int i;

	LCK_MTX_ASSERT(dn_mutex, LCK_MTX_ASSERT_OWNED);
	if (is64user) {
		pipesize = sizeof(struct dn_pipe_64);
		queuesize = sizeof(struct dn_flow_queue_64);
		setsize = sizeof(struct dn_flow_set_64);
	} else {
		pipesize = sizeof(struct dn_pipe_32);
		queuesize = sizeof(struct dn_flow_queue_32);
		setsize = sizeof(struct dn_flow_set_32);
	}
	  
                                                                 
    
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(p, &pipehash[i], next) {
			size += sizeof(*p) +
			    p->fs.rq_elements * sizeof(struct dn_flow_queue);
		}
		SLIST_FOREACH(set, &flowsethash[i], next) {
			size += sizeof(*set) +
			    set->rq_elements * sizeof(struct dn_flow_queue);
		}
	}
	return size;
}

static int
dummynet_get(struct sockopt *sopt)
{
	char *buf = NULL, *bp = NULL;                               
	size_t size = 0;
	struct dn_flow_set *set;
	struct dn_pipe *p;
	int error = 0, i;
	int is64user = 0;

	                            
	lck_mtx_lock(dn_mutex);
	  
                                                                
                                                      
    
	if (proc_is64bit(sopt->sopt_p)) {
		is64user = 1;
	}
	for (i = 0; i < 10; i++) {
		size = dn_calc_size(is64user);
		lck_mtx_unlock(dn_mutex);
		buf = _MALLOC(size, M_TEMP, M_WAITOK | M_ZERO);
		if (buf == NULL) {
			return ENOBUFS;
		}
		lck_mtx_lock(dn_mutex);
		if (size == dn_calc_size(is64user)) {
			break;
		}
		FREE(buf, M_TEMP);
		buf = NULL;
	}
	if (buf == NULL) {
		lck_mtx_unlock(dn_mutex);
		return ENOBUFS;
	}

	bp = buf;
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(p, &pipehash[i], next) {
			  
                                                  
                                                      
                                                  
                               
      
			if (is64user) {
				bp = cp_pipe_to_64_user(p,
				    (struct dn_pipe_64 *)bp);
			} else {
				bp = cp_pipe_to_32_user(p,
				    (struct dn_pipe_32 *)bp);
			}
		}
	}
	for (i = 0; i < HASHSIZE; i++) {
		SLIST_FOREACH(set, &flowsethash[i], next) {
			struct dn_flow_set_64 *fs_bp =
			    (struct dn_flow_set_64 *)bp;
			cp_flow_set_to_64_user(set, fs_bp);
			                            
			fs_bp->next = CAST_DOWN(user64_addr_t,
			    DN_IS_QUEUE);
			fs_bp->pipe = USER_ADDR_NULL;
			fs_bp->rq = USER_ADDR_NULL;
			bp += sizeof(struct dn_flow_set_64);
			bp = dn_copy_set_64( set, bp );
		}
	}
	lck_mtx_unlock(dn_mutex);
	error = sooptcopyout(sopt, buf, size);
	FREE(buf, M_TEMP);
	return error;
}

  
                                                                            
   
static int
ip_dn_ctl(struct sockopt *sopt)
{
	int error = 0;
	struct dn_pipe *p, tmp_pipe;

	                                                 
	if (sopt->sopt_dir == SOPT_SET && securelevel >= 3) {
		return EPERM;
	}

	switch (sopt->sopt_name) {
	default:
		printf("dummynet: -- unknown option %d", sopt->sopt_name);
		return EINVAL;

	case IP_DUMMYNET_GET:
		error = dummynet_get(sopt);
		break;

	case IP_DUMMYNET_FLUSH:
		dummynet_flush();
		break;

	case IP_DUMMYNET_CONFIGURE:
		p = &tmp_pipe;
		if (proc_is64bit(sopt->sopt_p)) {
			error = cp_pipe_from_user_64( sopt, p );
		} else {
			error = cp_pipe_from_user_32( sopt, p );
		}

		if (error) {
			break;
		}
		error = config_pipe(p);
		break;

	case IP_DUMMYNET_DEL:                               
		p = &tmp_pipe;
		if (proc_is64bit(sopt->sopt_p)) {
			error = cp_pipe_from_user_64( sopt, p );
		} else {
			error = cp_pipe_from_user_32( sopt, p );
		}
		if (error) {
			break;
		}

		error = delete_pipe(p);
		break;
	}
	return error;
}

void
dummynet_init(void)
{
	eventhandler_lists_ctxt_init(&dummynet_evhdlr_ctxt);
}

void
ip_dn_init(void)
{
	                 
	dn_mutex_grp_attr = lck_grp_attr_alloc_init();
	dn_mutex_grp = lck_grp_alloc_init("dn", dn_mutex_grp_attr);
	dn_mutex_attr = lck_attr_alloc_init();
	lck_mtx_init(dn_mutex, dn_mutex_grp, dn_mutex_attr);

	ready_heap.size = ready_heap.elements = 0;
	ready_heap.offset = 0;

	wfq_ready_heap.size = wfq_ready_heap.elements = 0;
	wfq_ready_heap.offset = 0;

	extract_heap.size = extract_heap.elements = 0;
	extract_heap.offset = 0;
	ip_dn_ctl_ptr = ip_dn_ctl;
	ip_dn_io_ptr = dummynet_io;
}

struct dn_event_nwk_wq_entry {
	struct nwk_wq_entry nwk_wqe;
	struct dummynet_event dn_ev_arg;
};

static void
dummynet_event_callback(void *arg)
{
	struct dummynet_event *p_dn_ev = (struct dummynet_event *)arg;

	EVENTHANDLER_INVOKE(&dummynet_evhdlr_ctxt, dummynet_event, p_dn_ev);
	return;
}

void
dummynet_event_enqueue_nwk_wq_entry(struct dummynet_event *p_dn_event)
{
	struct dn_event_nwk_wq_entry *p_dn_ev = NULL;

	MALLOC(p_dn_ev, struct dn_event_nwk_wq_entry *,
	    sizeof(struct dn_event_nwk_wq_entry),
	    M_NWKWQ, M_WAITOK | M_ZERO);

	p_dn_ev->nwk_wqe.func = dummynet_event_callback;
	p_dn_ev->nwk_wqe.is_arg_managed = TRUE;
	p_dn_ev->nwk_wqe.arg = &p_dn_ev->dn_ev_arg;

	bcopy(p_dn_event, &(p_dn_ev->dn_ev_arg),
	    sizeof(struct dummynet_event));
	nwk_wq_enqueue((struct nwk_wq_entry*)p_dn_ev);
}
