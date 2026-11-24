/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
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
 * Copyright (c) 1988, 1989, 1993
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
 *	@(#)radix.h	8.2 (Berkeley) 10/31/94
 * $FreeBSD: src/sys/net/radix.h,v 1.16.2.1 2000/05/03 19:17:11 wollman Exp $
 */

#ifndef _RADIX_H_
#define _RADIX_H_
#include <sys/appleapiopts.h>

#ifdef PRIVATE

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_RTABLE);
#endif

  
                                 
   

struct radix_node {
	struct  radix_mask *rn_mklist;                                          
	struct  radix_node *rn_parent;              
	short   rn_bit;                                                    
	char    rn_bmask;                                           
	u_char  rn_flags;                                    
#define RNF_NORMAL      1                                               
#define RNF_ROOT        2                                               
#define RNF_ACTIVE      4                                                    
	union {
		struct {                                             
			caddr_t rn_Key;                               
			caddr_t rn_Mask;                                 
			struct  radix_node *rn_Dupedkey;
		} rn_leaf;
		struct {                                             
			int     rn_Off;                                     
			struct  radix_node *rn_L;             
			struct  radix_node *rn_R;             
		} rn_node;
	}               rn_u;
#ifdef RN_DEBUG
	int rn_info;
	struct radix_node *rn_twin;
	struct radix_node *rn_ybro;
#endif

#if __arm__ && (__BIGGEST_ALIGNMENT__ > 4)
                                                                               
              
                                                                            
   
} __attribute__ ((aligned(8)));
#else
};
#endif

#define rn_dupedkey     rn_u.rn_leaf.rn_Dupedkey
#define rn_key          rn_u.rn_leaf.rn_Key
#define rn_mask         rn_u.rn_leaf.rn_Mask
#define rn_offset       rn_u.rn_node.rn_Off
#define rn_left         rn_u.rn_node.rn_L
#define rn_right        rn_u.rn_node.rn_R

  
                                                                        
   

struct radix_mask {
	short   rm_bit;                                                    
	char    rm_unused;                                
	u_char  rm_flags;                                 
	struct  radix_mask *rm_mklist;                         
	union   {
		caddr_t rmu_mask;                             
		struct  radix_node *rmu_leaf;                          
	}       rm_rmu;
	int     rm_refs;                                                    
};

#define rm_mask rm_rmu.rmu_mask
#define rm_leaf rm_rmu.rmu_leaf                                              


#define MKGet(m) {\
	if (rn_mkfreelist) {\
	        m = rn_mkfreelist; \
	        rn_mkfreelist = (m)->rm_mklist; \
	} else \
	        R_Malloc(m, struct radix_mask *, sizeof (*(m))); }\

#define MKFree(m) { (m)->rm_mklist = rn_mkfreelist; rn_mkfreelist = (m);}

typedef int walktree_f_t(struct radix_node *, void *);
typedef int rn_matchf_t(struct radix_node *, void *);

struct radix_node_head {
	struct  radix_node *rnh_treetop;
	int     rnh_addrsize;                                                   
	int     rnh_pktsize;                                                    
	struct  radix_node *(*rnh_addaddr)                                 
	(void *v, void *mask,
	    struct radix_node_head *head, struct radix_node nodes[]);
	struct  radix_node *(*rnh_addpkt)                                    
	(void *v, void *mask,
	    struct radix_node_head *head, struct radix_node nodes[]);
	struct  radix_node *(*rnh_deladdr)                                    
	(void *v, void *mask, struct radix_node_head *head);
	struct  radix_node *(*rnh_delpkt)                                       
	(void *v, void *mask, struct radix_node_head *head);
	struct  radix_node *(*rnh_matchaddr)                                  
	(void *v, struct radix_node_head *head);
	                                                
	struct  radix_node *(*rnh_matchaddr_args)
	(void *v, struct radix_node_head *head,
	    rn_matchf_t *f, void *w);
	struct  radix_node *(*rnh_lookup)                                     
	(void *v, void *mask, struct radix_node_head *head);
	                                                      
	struct  radix_node *(*rnh_lookup_args)
	(void *v, void *mask, struct radix_node_head *head,
	    rn_matchf_t *f, void *);
	struct  radix_node *(*rnh_matchpkt)                                     
	(void *v, struct radix_node_head *head);
	int     (*rnh_walktree)                                    
	(struct radix_node_head *head, walktree_f_t *f, void *w);
	int     (*rnh_walktree_from)                                       
	(struct radix_node_head *head, void *a, void *m,
	walktree_f_t *f, void *w);
	void    (*rnh_close)                                              
	(struct radix_node *rn, struct radix_node_head *head);
	struct  radix_node rnh_nodes[3];                                        
	int     rnh_cnt;                                            
};

#ifndef KERNEL
#define Bcmp(a, b, n) bcmp(((char *)(a)), ((char *)(b)), (n))
#define Bcopy(a, b, n) bcopy(((char *)(a)), ((char *)(b)), (unsigned)(n))
#define Bzero(p, n) bzero((char *)(p), (int)(n));
#define R_Malloc(p, t, n) (p = (t) malloc((unsigned int)(n)))
#define R_Free(p) free((char *)p);
#else
#define Bcmp(a, b, n) bcmp(((caddr_t)(a)), ((caddr_t)(b)), (unsigned)(n))
#define Bcopy(a, b, n) bcopy(((caddr_t)(a)), ((caddr_t)(b)), (unsigned)(n))
#define Bzero(p, n) bzero((caddr_t)(p), (unsigned)(n));
#define R_Malloc(p, t, n) (p = (t) _MALLOC((uint32_t)(n), M_RTABLE, M_WAITOK))
#define R_Free(p) _FREE((caddr_t)p, M_RTABLE);
#endif           

void     rn_init(void);
int      rn_inithead(void **, int);
int      rn_refines(void *, void *);
struct radix_node
*rn_addmask(void *, int, int),
*rn_addroute(void *, void *, struct radix_node_head *,
    struct radix_node [2]),
*rn_delete(void *, void *, struct radix_node_head *),
*rn_lookup(void *v_arg, void *m_arg, struct radix_node_head *head),
*rn_lookup_args(void *v_arg, void *m_arg, struct radix_node_head *head,
    rn_matchf_t *, void *),
*rn_match(void *, struct radix_node_head *),
*rn_match_args(void *, struct radix_node_head *, rn_matchf_t *, void *);

#endif              
#endif                
