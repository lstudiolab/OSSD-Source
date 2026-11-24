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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
 * $FreeBSD: src/sys/netinet/ip_dummynet.h,v 1.32 2004/08/17 22:05:54 andre Exp $
 */

#ifndef _IP_DUMMYNET_H
#define _IP_DUMMYNET_H

#include <sys/appleapiopts.h>

#ifdef PRIVATE
#include <netinet/ip_flowid.h>

                                  
#define APPLY_MASK(addr, mask)                          \
    (addr)->__u6_addr.__u6_addr32[0] &= (mask)->__u6_addr.__u6_addr32[0]; \
    (addr)->__u6_addr.__u6_addr32[1] &= (mask)->__u6_addr.__u6_addr32[1]; \
    (addr)->__u6_addr.__u6_addr32[2] &= (mask)->__u6_addr.__u6_addr32[2]; \
    (addr)->__u6_addr.__u6_addr32[3] &= (mask)->__u6_addr.__u6_addr32[3];

  
                                                                       
                                                                        
                                                                        
                                     
   

  
                                                                      
                           
  
                                                         
  
                                                         
  
                                                                       
                                                                   
                         
                                                                 
                                                                   
                                         
                                                                     
                                             
                                               
                                                                  
                                      
   
typedef u_int64_t dn_key;                        
#define DN_KEY_LT(a, b)     ((int64_t)((a)-(b)) < 0)
#define DN_KEY_LEQ(a, b)    ((int64_t)((a)-(b)) <= 0)
#define DN_KEY_GT(a, b)     ((int64_t)((a)-(b)) > 0)
#define DN_KEY_GEQ(a, b)    ((int64_t)((a)-(b)) >= 0)
#define MAX64(x, y)  (( (int64_t) ( (y)-(x) )) > 0 ) ? (y) : (x)
#define MY_M    16                                                        

  
                                                                      
                                    
   

  
                                                                      
        
   
#define DN_MAX_HASH_SIZE 65536

  
                                                            
                             
                                                                        
                                                           
                                                                           
                                                                  
                                                                     
                                                                  
                                                                     
                                                                  
                                                                  
                                                                   
                                                             
   
struct dn_heap_entry {
	dn_key key;                                                       
	void *object;                       
};

struct dn_heap {
	int size;
	int elements;
	int offset;                                                         
	struct dn_heap_entry *p;                                        
};

  
                                                                 
                                                               
                                                                 
                           
   
#ifdef KERNEL
#include <net/if_var.h>
#include <net/route.h>
#include <netinet/ip_var.h>                          
#include <netinet/ip6.h>                              
#include <netinet/in.h>
#include <netinet6/ip6_var.h>                         

struct dn_pkt_tag {
	void                *dn_pf_rule;                              
	int                 dn_dir;                                                
#define DN_TO_IP_OUT    1
#define DN_TO_IP_IN     2
#define DN_TO_BDG_FWD   3
#define DN_TO_IP6_IN    4
#define DN_TO_IP6_OUT   5
	dn_key              dn_output_time;                                           
	struct ifnet        *dn_ifp;                                              
	union {
		struct sockaddr_in      _dn_dst;
		struct sockaddr_in6     _dn_dst6;
	}                   dn_dst_;
#define dn_dst dn_dst_._dn_dst
#define dn_dst6 dn_dst_._dn_dst6
	union {
		struct route            _dn_ro;                                      
		struct route_in6        _dn_ro6;                                      
	}               dn_ro_;
#define dn_ro dn_ro_._dn_ro
#define dn_ro6 dn_ro_._dn_ro6
	struct route_in6    dn_ro6_pmtu;                            
	struct ifnet        *dn_origifp;                            
	u_int32_t           dn_mtu;                                 
	u_int32_t           dn_unfragpartlen;                       
	struct ip6_exthdrs  dn_exthdrs;                             
	int                 dn_flags;                                        
	union {
		struct ip_out_args      _dn_ipoa;                                           
		struct ip6_out_args     _dn_ip6oa;                                           
	}                   dn_ipoa_;
#define dn_ipoa dn_ipoa_._dn_ipoa
#define dn_ip6oa dn_ipoa_._dn_ip6oa
};
#else
struct dn_pkt;
#endif             

  
                                              
  
                                                                         
                                            
  
                                                                       
                                                                    
                                                                    
                                                                 
                                                                   
         
  
                                                              
                                                                 
                                                                   
                                                                   
                                                      
  
                                                               
                                                                  
                                                                  
                       
  
                                                                     
                                                                       
                                                  
                                                                      
                                                                   
                         
  
                                                                            
  
                                                                    
                           
                                                        
                                     
                                                                      
  
                                                                     
                                                        
                                                                     
                          
  
                                                                  
                                                                   
  
                                                                       
                                                                       
                                                                  
                                                 
  
                                                                       
                                                                   
                                                                    
                                                              
  
  
   

  
                                                               
                                                                    
         
  
                                                                   
                      
   
struct dn_flow_queue {
	struct dn_flow_queue *next;
	struct ip_flow_id id;

	struct mbuf *head, *tail;                       
	u_int len;
	u_int len_bytes;
	u_int32_t numbytes;                                                           

	u_int64_t tot_pkts;                              
	u_int64_t tot_bytes;
	u_int32_t drops;

	int hash_slot;                                    

	                    
	int avg;                                                        
	int count;                                                
	int random;                                        
	u_int32_t q_time;                                     

	                   
	struct dn_flow_set *fs;                      
	int heap_pos;                                                   
	dn_key sched_time;                                                     

	dn_key S, F;                                         
	  
                                                              
                                         
    
};

  
                                                                  
                                                                          
  
                                                                 
                                                               
                                                                      
         
  
                                                                           
                                                                    
   
struct dn_flow_set {
	SLIST_ENTRY(dn_flow_set)    next;                                

	u_short fs_nr;                                     
	u_short flags_fs;
#define DN_HAVE_FLOW_MASK       0x0001
#define DN_IS_RED               0x0002
#define DN_IS_GENTLE_RED        0x0004
#define DN_QSIZE_IS_BYTES       0x0008                                       
#define DN_NOERROR              0x0010                                       
#define DN_IS_PIPE              0x4000
#define DN_IS_QUEUE             0x8000

	struct dn_pipe *pipe;                               
	u_short parent_nr;                                              

	int weight;                                   
	int qsize;                                                
	int plr;                                                       

	struct ip_flow_id flow_mask;

	                                             
	int rq_size;                                 
	int rq_elements;                             
	struct dn_flow_queue **rq;                               

	u_int32_t last_expired;                                   
	int backlogged;                                              

	                    
#define SCALE_RED               16
#define SCALE(x)                ( (x) << SCALE_RED )
#define SCALE_VAL(x)            ( (x) >> SCALE_RED )
#define SCALE_MUL(x, y)          ( ( (x) * (y) ) >> SCALE_RED )
	int w_q;                                           
	int max_th;                                                       
	int min_th;                                                       
	int max_p;                                                  
	u_int c_1;                                                  
	u_int c_2;                                                         
	u_int c_3;                                                       
	u_int c_4;                                                  
	u_int * w_q_lookup;                                               
	u_int lookup_depth;                                
	int lookup_step;                                                 
	int lookup_weight;                                              
	int avg_pkt_size;                               
	int max_pkt_size;                            
};

SLIST_HEAD(dn_flow_set_head, dn_flow_set);

  
                                                                 
                                               
  
                                                               
                                                             
                                               
                                                                  
               
                                                              
                                                       
                                
  
   
struct dn_pipe {                            
	SLIST_ENTRY(dn_pipe)        next;                                

	int pipe_nr;                        
	int bandwidth;                                   
	int delay;                                 

	struct      mbuf *head, *tail;                             

	           
	struct dn_heap scheduler_heap;                                   
	struct dn_heap not_eligible_heap;                                  
	struct dn_heap idle_heap;                                             

	dn_key V;                                 
	int sum;                                                           
	int numbytes;                                                    

	dn_key sched_time;                                                 

	  
                                                                           
                                                                           
    
	char if_name[IFNAMSIZ];
	struct ifnet *ifp;
	int ready;                                                     

	struct dn_flow_set fs;                                 
};

SLIST_HEAD(dn_pipe_head, dn_pipe);

#ifdef BSD_KERNEL_PRIVATE
extern uint32_t my_random(void);
void ip_dn_init(void);

typedef int ip_dn_ctl_t(struct sockopt *);               
typedef int ip_dn_io_t(struct mbuf *m, int pipe_nr, int dir,
    struct ip_fw_args *fwa);
extern  ip_dn_ctl_t *ip_dn_ctl_ptr;
extern  ip_dn_io_t *ip_dn_io_ptr;
#define DUMMYNET_LOADED (ip_dn_io_ptr != NULL)

#pragma pack(4)

struct dn_heap_32 {
	int size;
	int elements;
	int offset;                                                         
	user32_addr_t p;                                        
};

struct dn_flow_queue_32 {
	user32_addr_t next;
	struct ip_flow_id id;

	user32_addr_t head, tail;                       
	u_int len;
	u_int len_bytes;
	u_int32_t numbytes;                                                   

	u_int64_t tot_pkts;                              
	u_int64_t tot_bytes;
	u_int32_t drops;

	int hash_slot;                                            

	                    
	int avg;                                                        
	int count;                                                
	int random;                                        
	u_int32_t q_time;                                     

	                   
	user32_addr_t fs;                      
	int heap_pos;                                                   
	dn_key sched_time;                                                     

	dn_key S, F;                                         
	  
                                                              
                                         
    
};

struct dn_flow_set_32 {
	user32_addr_t       next;                                         

	u_short fs_nr;                                             
	u_short flags_fs;
#define DN_HAVE_FLOW_MASK       0x0001
#define DN_IS_RED               0x0002
#define DN_IS_GENTLE_RED        0x0004
#define DN_QSIZE_IS_BYTES       0x0008                                       
#define DN_NOERROR              0x0010                                               
#define DN_IS_PIPE              0x4000
#define DN_IS_QUEUE             0x8000

	user32_addr_t pipe;                                 
	u_short parent_nr;                                              

	int weight;                                   
	int qsize;                                                
	int plr;                                                       

	struct ip_flow_id flow_mask;

	                                             
	int rq_size;                                 
	int rq_elements;                      
	user32_addr_t rq;                               

	u_int32_t last_expired;                                   
	int backlogged;                                                      

	                    
#define SCALE_RED               16
#define SCALE(x)                ( (x) << SCALE_RED )
#define SCALE_VAL(x)            ( (x) >> SCALE_RED )
#define SCALE_MUL(x, y)          ( ( (x) * (y) ) >> SCALE_RED )
	int w_q;                                           
	int max_th;                                                       
	int min_th;                                                       
	int max_p;                                                  
	u_int c_1;                                                  
	u_int c_2;                                                         
	u_int c_3;                                                       
	u_int c_4;                                                  
	user32_addr_t w_q_lookup;                                           
	u_int lookup_depth;                                
	int lookup_step;                                                 
	int lookup_weight;                                              
	int avg_pkt_size;                               
	int max_pkt_size;                            
};

struct dn_pipe_32 {                         
	user32_addr_t       next;

	int pipe_nr;                        
	int bandwidth;                                   
	int delay;                                 

	user32_addr_t head, tail;                            

	           
	struct dn_heap_32 scheduler_heap;                                   
	struct dn_heap_32 not_eligible_heap;                                  
	struct dn_heap_32 idle_heap;                                             

	dn_key V;                                 
	int sum;                                                           
	int numbytes;                                                    

	dn_key sched_time;                                            

	  
                                                                           
                                                                           
    
	char if_name[IFNAMSIZ];
	user32_addr_t ifp;
	int ready;                                                                  

	struct dn_flow_set_32 fs;                                 
};
#pragma pack()


struct dn_heap_64 {
	int size;
	int elements;
	int offset;                                                         
	user64_addr_t p;                                        
};


struct dn_flow_queue_64 {
	user64_addr_t next;
	struct ip_flow_id id;

	user64_addr_t head, tail;                       
	u_int len;
	u_int len_bytes;
	u_int32_t numbytes;                                                           

	u_int64_t tot_pkts;                                      
	u_int64_t tot_bytes;
	u_int32_t drops;

	int hash_slot;                                                    

	                    
	int avg;                                                        
	int count;                                                
	int random;                                        
	u_int32_t q_time;                                     

	                   
	user64_addr_t fs;                                    
	int heap_pos;                                                                   
	dn_key sched_time;                                                             

	dn_key S, F;                                                         
	  
                                                              
                                         
    
};

struct dn_flow_set_64 {
	user64_addr_t next;                                                      

	u_short fs_nr;                                     
	u_short flags_fs;
#define DN_HAVE_FLOW_MASK       0x0001
#define DN_IS_RED               0x0002
#define DN_IS_GENTLE_RED        0x0004
#define DN_QSIZE_IS_BYTES       0x0008                                       
#define DN_NOERROR              0x0010                                               
#define DN_IS_PIPE              0x4000
#define DN_IS_QUEUE             0x8000

	user64_addr_t pipe;                                 
	u_short parent_nr;                                              

	int weight;                                   
	int qsize;                                                
	int plr;                                                       

	struct ip_flow_id flow_mask;

	                                             
	int rq_size;                                 
	int rq_elements;                      
	user64_addr_t rq;                               

	u_int32_t last_expired;                                   
	int backlogged;                                                      

	                    
#define SCALE_RED               16
#define SCALE(x)                ( (x) << SCALE_RED )
#define SCALE_VAL(x)            ( (x) >> SCALE_RED )
#define SCALE_MUL(x, y)          ( ( (x) * (y) ) >> SCALE_RED )
	int w_q;                                           
	int max_th;                                                       
	int min_th;                                                       
	int max_p;                                                  
	u_int c_1;                                                  
	u_int c_2;                                                         
	u_int c_3;                                                       
	u_int c_4;                                                  
	user64_addr_t w_q_lookup;                                           
	u_int lookup_depth;                                
	int lookup_step;                                                 
	int lookup_weight;                                              
	int avg_pkt_size;                               
	int max_pkt_size;                            
};

struct dn_pipe_64 {                         
	user64_addr_t       next;

	int pipe_nr;                        
	int bandwidth;                                   
	int delay;                                 

	user64_addr_t head, tail;                            

	           
	struct dn_heap_64 scheduler_heap;                                         
	struct dn_heap_64 not_eligible_heap;                                     
	struct dn_heap_64 idle_heap;                                                                

	dn_key V;                                 
	int sum;                                                           
	int numbytes;                                                    

	dn_key sched_time;                                            

	  
                                                                           
                                                                           
    
	char if_name[IFNAMSIZ];
	user64_addr_t ifp;
	int ready;                                                     

	struct dn_flow_set_64 fs;                                 
};

#include <sys/eventhandler.h>
                                          
extern struct eventhandler_lists_ctxt dummynet_evhdlr_ctxt;
extern void dummynet_init(void);

struct dn_pipe_mini_config {
	uint32_t bandwidth;
	uint32_t delay;
	uint32_t plr;
};

struct dn_rule_mini_config {
	uint32_t dir;
	uint32_t af;
	uint32_t proto;
	  
                                                    
                                                              
                      
                                                              
                                      
                                              
    
	uint32_t src_port;
	uint32_t dst_port;
	char ifname[IFXNAMSIZ];
};

struct dummynet_event {
	uint32_t dn_event_code;
	union {
		struct dn_pipe_mini_config _dnev_pipe_config;
		struct dn_rule_mini_config _dnev_rule_config;
	} dn_event;
};

#define dn_event_pipe_config    dn_event._dnev_pipe_config
#define dn_event_rule_config    dn_event._dnev_rule_config

extern void dummynet_event_enqueue_nwk_wq_entry(struct dummynet_event *);

enum {
	DUMMYNET_RULE_CONFIG,
	DUMMYNET_RULE_DELETE,
	DUMMYNET_PIPE_CONFIG,
	DUMMYNET_PIPE_DELETE,
	DUMMYNET_NLC_DISABLED,
};

enum    { DN_INOUT, DN_IN, DN_OUT };
  
                                     
                                  
                                                   
   
typedef void (*dummynet_event_fn) (struct eventhandler_entry_arg, struct dummynet_event *);
EVENTHANDLER_DECLARE(dummynet_event, dummynet_event_fn);
#endif                         
#endif              
#endif                     
