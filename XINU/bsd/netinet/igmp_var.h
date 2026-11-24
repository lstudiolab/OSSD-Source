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
 * Copyright (c) 1988 Stephen Deering.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	from: @(#)igmp_var.h	8.1 (Berkeley) 7/19/93
 */

#ifndef _NETINET_IGMP_VAR_H_
#define _NETINET_IGMP_VAR_H_
#include <sys/appleapiopts.h>

  
                                             
                                       
  
                                                
  
                              
   

struct igmpstat_v3 {
	  
                                               
    
	uint32_t igps_version;                                         
	uint32_t igps_len;                                            
	  
                       
    
	uint64_t igps_rcv_total;                                          
	uint64_t igps_rcv_tooshort;                                      
	uint64_t igps_rcv_badttl;                                           
	uint64_t igps_rcv_badsum;                                       
	  
                     
    
	uint64_t igps_rcv_v1v2_queries;                                     
	uint64_t igps_rcv_v3_queries;                                
	uint64_t igps_rcv_badqueries;                                 
	uint64_t igps_rcv_gen_queries;                                
	uint64_t igps_rcv_group_queries;                            
	uint64_t igps_rcv_gsr_queries;                                     
	uint64_t igps_drop_gsr_queries;                                   
	  
                      
    
	uint64_t igps_rcv_reports;                                       
	uint64_t igps_rcv_badreports;                                 
	uint64_t igps_rcv_ourreports;                                        
	uint64_t igps_rcv_nora;                                               
	uint64_t igps_snd_reports;                                   
	  
                                 
    
	uint64_t __igps_pad[4];
} __attribute__((aligned(8)));

  
                                                       
  
   

struct igmpstat {
	u_int   igps_rcv_total;                                           
	u_int   igps_rcv_tooshort;                                       
	u_int   igps_rcv_badsum;                                        
	u_int   igps_rcv_queries;                                        
	u_int   igps_rcv_badqueries;                                  
	u_int   igps_rcv_reports;                                        
	u_int   igps_rcv_badreports;                                  
	u_int   igps_rcv_ourreports;                                         
	u_int   igps_snd_reports;                                    
};

#define IGPS_VERSION_3  3
#define IGPS_VERSION3_LEN               168

#ifdef PRIVATE
  
                                                 
   
#ifndef XNU_KERNEL_PRIVATE
struct igmp_ifinfo {
#else
struct igmp_ifinfo_u {
#endif                         
	uint32_t igi_ifindex;                                           
	uint32_t igi_version;                                       
	uint32_t igi_v1_timer;                                        
	uint32_t igi_v2_timer;                                        
	uint32_t igi_v3_timer;                                                 
	uint32_t igi_flags;                                   
	uint32_t igi_rv;                                        
	uint32_t igi_qi;                                       
	uint32_t igi_qri;                                               
	uint32_t igi_uri;                                                   
};

#define IGIF_SILENT     0x00000001                                       
#define IGIF_LOOPBACK   0x00000002                                         

  
                    
   
#define IGMP_VERSION_NONE               0              
#define IGMP_VERSION_1                  1
#define IGMP_VERSION_2                  2
#define IGMP_VERSION_3                  3              
#endif              

#ifdef BSD_KERNEL_PRIVATE
#include <libkern/libkern.h>
#define IGMP_DEBUG 1
#ifdef IGMP_DEBUG
extern int igmp_debug;

#define IGMP_PRINTF(x)  do { if (igmp_debug) printf x; } while (0)
#define IGMP_INET_PRINTF(addr, x) do {                                  \
	if (igmp_debug) {                                               \
	        char _igmp_inet_buf[MAX_IPv4_STR_LEN];                  \
	        inet_ntop(AF_INET, &(addr), _igmp_inet_buf,             \
	            sizeof(_igmp_inet_buf));                            \
	        printf x;                                               \
	}                                                               \
} while (0)
#else
#define IGMP_PRINTF(x)
#endif

#define OIGMPSTAT_ADD(name, val)        atomic_add_32(&igmpstat.name , (val))
#define OIGMPSTAT_INC(name)             OIGMPSTAT_ADD(name, 1)

#define IGMPSTAT_ADD(name, val)         atomic_add_64(&igmpstat_v3.name , (val))
#define IGMPSTAT_INC(name)              IGMPSTAT_ADD(name, 1)

#define IGMP_RANDOM_DELAY(X) (random() % (X) + 1)

#define IGMP_MAX_STATE_CHANGES          24                                    

  
                         
   
#define IGMP_NOT_MEMBER                 0                                   
#define IGMP_SILENT_MEMBER              1                                    
#define IGMP_REPORTING_MEMBER           2                                 
#define IGMP_IDLE_MEMBER                3                                
#define IGMP_LAZY_MEMBER                4                                      
#define IGMP_SLEEPING_MEMBER            5                                    
#define IGMP_AWAKENING_MEMBER           6                                      
#define IGMP_G_QUERY_PENDING_MEMBER     7                                 
#define IGMP_SG_QUERY_PENDING_MEMBER    8                                  
#define IGMP_LEAVING_MEMBER             9                                      
                                                                             
  
                                     
   
#define IGMP_RV_INIT            2                                
#define IGMP_RV_MIN             1
#define IGMP_RV_MAX             7

#define IGMP_QI_INIT            125                             
#define IGMP_QI_MIN             1
#define IGMP_QI_MAX             255

#define IGMP_QRI_INIT           10                                       
#define IGMP_QRI_MIN            1
#define IGMP_QRI_MAX            255

#define IGMP_URI_INIT           3                                            
#define IGMP_URI_MIN            0
#define IGMP_URI_MAX            10

#define IGMP_MAX_G_GS_PACKETS           8                                  
#define IGMP_MAX_STATE_CHANGE_PACKETS   8                                    
#define IGMP_MAX_RESPONSE_PACKETS       16                                     
#define IGMP_MAX_RESPONSE_BURST         4                                     
#define IGMP_RESPONSE_BURST_INTERVAL    1               

  
                            
   
#define M_IGMPV2        M_PROTO1                              
#define M_IGMPV3_HDR    M_PROTO2                                       
#define M_GROUPREC      M_PROTO3                                          
#define M_IGMP_LOOP     M_LOOP                                              

  
                                                                
                                                                  
                                           
   
#define RAOPT_LEN       4                                                     
#define IGMP_LEADINGSPACE               \
	(sizeof(struct ip) + RAOPT_LEN + sizeof(struct igmp_report))

struct igmp_ifinfo {
	decl_lck_mtx_data(, igi_lock);
	uint32_t igi_refcnt;                         
	uint32_t igi_debug;                              
	LIST_ENTRY(igmp_ifinfo) igi_link;
	struct ifnet *igi_ifp;                                          
	uint32_t igi_version;                                       
	uint32_t igi_v1_timer;                                        
	uint32_t igi_v2_timer;                                        
	uint32_t igi_v3_timer;                                                 
	uint32_t igi_flags;                                   
	uint32_t igi_rv;                                        
	uint32_t igi_qi;                                       
	uint32_t igi_qri;                                               
	uint32_t igi_uri;                                                   
	SLIST_HEAD(, in_multi)   igi_relinmhead;                      
	struct ifqueue   igi_gq;                                              
	struct ifqueue   igi_v2q;                             
};

#define IGI_LOCK_ASSERT_HELD(_igi)                                      \
	LCK_MTX_ASSERT(&(_igi)->igi_lock, LCK_MTX_ASSERT_OWNED)

#define IGI_LOCK_ASSERT_NOTHELD(_igi)                                   \
	LCK_MTX_ASSERT(&(_igi)->igi_lock, LCK_MTX_ASSERT_NOTOWNED)

#define IGI_LOCK(_igi)                                                  \
	lck_mtx_lock(&(_igi)->igi_lock)

#define IGI_LOCK_SPIN(_igi)                                             \
	lck_mtx_lock_spin(&(_igi)->igi_lock)

#define IGI_CONVERT_LOCK(_igi) do {                                     \
	IGI_LOCK_ASSERT_HELD(_igi);                                     \
	lck_mtx_convert_spin(&(_igi)->igi_lock);                        \
} while (0)

#define IGI_UNLOCK(_igi)                                                \
	lck_mtx_unlock(&(_igi)->igi_lock)

#define IGI_ADDREF(_igi)                                                \
	igi_addref(_igi, 0)

#define IGI_ADDREF_LOCKED(_igi)                                         \
	igi_addref(_igi, 1)

#define IGI_REMREF(_igi)                                                \
	igi_remref(_igi)

  
                         
   
#define IGMP_IFINFO(ifp)        ((ifp)->if_igi)

  
                                 
   
struct igmp_tparams {
	int     qpt;                                        
	int     it;                                   
	int     cst;                                      
	int     sct;                                     
};

extern void igmp_init(struct protosw *, struct domain *);
extern int igmp_change_state(struct in_multi *, struct igmp_tparams *);
extern struct igmp_ifinfo *igmp_domifattach(struct ifnet *, zalloc_flags_t);
extern void igmp_domifreattach(struct igmp_ifinfo *);
extern void igmp_domifdetach(struct ifnet *);
extern void igmp_input(struct mbuf *, int);
extern int igmp_joingroup(struct in_multi *);
extern void igmp_leavegroup(struct in_multi *);
extern void igmp_set_timeout(struct igmp_tparams *);
extern void igi_addref(struct igmp_ifinfo *, int);
extern void igi_remref(struct igmp_ifinfo *);
__private_extern__ void igmp_initsilent(struct ifnet *, struct igmp_ifinfo *);

SYSCTL_DECL(_net_inet_igmp);

#endif                         

  
                                
   
#define IGMPCTL_STATS           1                                   
#define IGMPCTL_MAXID           2

#ifdef BSD_KERNEL_PRIVATE
#define IGMPCTL_NAMES { \
	{ 0, 0 }, \
	{ "stats", CTLTYPE_STRUCT }, \
}

#endif                         
#endif
