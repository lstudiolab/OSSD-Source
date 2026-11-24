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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

                                                                                    
                                                                

/*
 * Copyright (C) 1998 WIDE Project.
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

#ifndef _NETINET6_MLD6_VAR_H_
#define _NETINET6_MLD6_VAR_H_
#include <sys/appleapiopts.h>

  
                                     
                                       
   

#ifdef PRIVATE
  
                      
   
#ifndef XNU_KERNEL_PRIVATE
struct mld_ifinfo {
#else
struct mld_ifinfo_u {
#endif                         
	uint32_t mli_ifindex;                                           
	uint32_t mli_version;                                      
	uint32_t mli_v1_timer;                                       
	uint32_t mli_v2_timer;                                                
	uint32_t mli_flags;                                  
	uint32_t mli_rv;                                       
	uint32_t mli_qi;                                      
	uint32_t mli_qri;                                              
	uint32_t mli_uri;                                                  
	uint32_t _pad;
};

#define MLIF_SILENT     0x00000001                                      
#define MLIF_USEALLOW   0x00000002                                            
#define MLIF_PROCESSED  0x00000004                                                       

  
                   
   
#define MLD_VERSION_NONE                0              
#define MLD_VERSION_1                   1
#define MLD_VERSION_2                   2              
#endif              

#ifdef BSD_KERNEL_PRIVATE
#include <sys/syslog.h>

#define MLD_DEBUG 1
#ifdef MLD_DEBUG
extern int mld_debug;
#define MLD_PRINTF(x)   do { if (mld_debug) printf x; } while (0)
#else
#define MLD_PRINTF(x)
#endif

#define MLD_RANDOM_DELAY(X)             (RandomULong() % (X) + 1)
#define MLD_MAX_STATE_CHANGES           24                                    

  
                        
   
#define MLD_NOT_MEMBER                  0                                
#define MLD_SILENT_MEMBER               1                                   
#define MLD_REPORTING_MEMBER            2                            
#define MLD_IDLE_MEMBER                 3                             
#define MLD_LAZY_MEMBER                 4                                   
#define MLD_SLEEPING_MEMBER             5                                 
#define MLD_AWAKENING_MEMBER            6                                   
#define MLD_G_QUERY_PENDING_MEMBER      7                                
#define MLD_SG_QUERY_PENDING_MEMBER     8                                 
#define MLD_LEAVING_MEMBER              9                                     
                                                                             
  
                                    
   
#define MLD_RV_INIT             2                                
#define MLD_RV_MIN              1
#define MLD_RV_MAX              7

#define MLD_QI_INIT             125                             
#define MLD_QI_MIN              1
#define MLD_QI_MAX              255

#define MLD_QRI_INIT            10                                       
#define MLD_QRI_MIN             1
#define MLD_QRI_MAX             255

#define MLD_URI_INIT            3                                            
#define MLD_URI_MIN             0
#define MLD_URI_MAX             10

#define MLD_MAX_GS_SOURCES              256                                  
#define MLD_MAX_G_GS_PACKETS            8                                  
#define MLD_MAX_STATE_CHANGE_PACKETS    8                                    
#define MLD_MAX_RESPONSE_PACKETS        16                                     
#define MLD_MAX_RESPONSE_BURST          4                                     
#define MLD_RESPONSE_BURST_INTERVAL     1               

  
                           
   
#define M_MLDV1         M_PROTO1                             
#define M_GROUPREC      M_PROTO3                                          

  
                                              
  
                                                                   
                                                                   
                                                                   
                                                                
  
                                                                    
                   
   
#define MLD_MTUSPACE    (sizeof(struct ip6_hdr) + sizeof(struct mld_raopt) + \
	                 sizeof(struct icmp6_hdr))

struct mld_ifinfo {
	decl_lck_mtx_data(, mli_lock);
	uint32_t mli_refcnt;                         
	uint32_t mli_debug;                              
	LIST_ENTRY(mld_ifinfo) mli_link;
	struct ifnet *mli_ifp;                                          
	uint32_t mli_version;                                      
	uint32_t mli_v1_timer;                                       
	uint32_t mli_v2_timer;                                                
	uint32_t mli_flags;                                  
	uint32_t mli_rv;                                       
	uint32_t mli_qi;                                      
	uint32_t mli_qri;                                              
	uint32_t mli_uri;                                                  
	SLIST_HEAD(, in6_multi)  mli_relinmhead;                      
	struct ifqueue   mli_gq;                                              
	struct ifqueue   mli_v1q;                          
};

#define MLI_LOCK_ASSERT_HELD(_mli)                                      \
	LCK_MTX_ASSERT(&(_mli)->mli_lock, LCK_MTX_ASSERT_OWNED)

#define MLI_LOCK_ASSERT_NOTHELD(_mli)                                   \
	LCK_MTX_ASSERT(&(_mli)->mli_lock, LCK_MTX_ASSERT_NOTOWNED)

#define MLI_LOCK(_mli)                                                  \
	lck_mtx_lock(&(_mli)->mli_lock)

#define MLI_LOCK_SPIN(_mli)                                             \
	lck_mtx_lock_spin(&(_mli)->mli_lock)

#define MLI_CONVERT_LOCK(_mli) do {                                     \
	MLI_LOCK_ASSERT_HELD(_mli);                                     \
	lck_mtx_convert_spin(&(_mli)->mli_lock);                        \
} while (0)

#define MLI_UNLOCK(_mli)                                                \
	lck_mtx_unlock(&(_mli)->mli_lock)

#define MLI_ADDREF(_mli)                                                \
	mli_addref(_mli, 0)

#define MLI_ADDREF_LOCKED(_mli)                                         \
	mli_addref(_mli, 1)

#define MLI_REMREF(_mli)                                                \
	mli_remref(_mli)

  
                        
   
#define MLD_IFINFO(ifp) ((ifp)->if_mli)

  
                                
   
struct mld_tparams {
	int     qpt;                                         
	int     it;                                    
	int     cst;                                       
	int     sct;                                      
};

extern int mld_change_state(struct in6_multi *, struct mld_tparams *,
    const int);
extern struct mld_ifinfo *mld_domifattach(struct ifnet *, zalloc_flags_t);
extern void mld_domifreattach(struct mld_ifinfo *);
extern void mld_domifdetach(struct ifnet *);
extern void mld_fasttimo(void);
extern void mld_ifdetach(struct ifnet *);
extern int mld_input(struct mbuf *, int, int);
extern void mld_init(void);
extern void mld_set_timeout(struct mld_tparams *);
extern void mli_addref(struct mld_ifinfo *, int);
extern void mli_remref(struct mld_ifinfo *);
__private_extern__ void mld6_initsilent(struct ifnet *, struct mld_ifinfo *);

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet6_mld);
#endif

#endif                         
#endif                            
