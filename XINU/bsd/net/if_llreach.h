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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _NET_IF_LLREACH_H_
#define _NET_IF_LLREACH_H_

#ifdef PRIVATE
#ifdef  __cplusplus
extern "C" {
#endif

#include <sys/types.h>

  
                                                               
   
#define IF_LLREACHINFO_ADDRLEN          64                           
#define IF_LLREACHINFO_RESERVED2        16                              

struct if_llreach_info {
	u_int32_t               lri_refcnt;                          
	u_int32_t               lri_ifindex;                         
	u_int64_t               lri_expire;                                     
	u_int32_t               lri_probes;                            
	u_int16_t               lri_reserved;                       
	u_int16_t               lri_proto;                    
	u_int8_t                lri_addr[IF_LLREACHINFO_ADDRLEN];              
	int32_t                 lri_rssi;                                     
	int32_t                 lri_lqm;                                 
	int32_t                 lri_npm;                                   
	u_int8_t                lri_reserved2[IF_LLREACHINFO_RESERVED2];
};

#ifdef XNU_KERNEL_PRIVATE
#include <sys/tree.h>
#include <kern/locks.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

  
                                                                                 
   
#define IF_LLREACH_MAXLEN       ETHER_ADDR_LEN

struct if_llreach {
	decl_lck_mtx_data(, lr_lock);
	RB_ENTRY(if_llreach)    lr_link;                           
	struct ifnet            *lr_ifp;                                   
	u_int32_t               lr_refcnt;                           
	u_int32_t               lr_reqcnt;                                 
	u_int32_t               lr_debug;                                
	u_int32_t               lr_probes;                                   
	u_int64_t               lr_basecal;                             
	u_int64_t               lr_baseup;                       
	u_int64_t               lr_lastrcvd;                                 
	u_int32_t               lr_basereachable;                    
	u_int32_t               lr_reachable;                       
	struct lr_key_s {
		u_int16_t       proto;                        
		u_int8_t        addr[IF_LLREACH_MAXLEN];              
	} lr_key;
	int32_t                 lr_rssi;                                      
	int32_t                 lr_lqm;                                  
	int32_t                 lr_npm;                                    
};

RB_PROTOTYPE_SC_PREV(__private_extern__, ll_reach_tree, if_llreach,
    ls_link, ifllr_cmp);

#define IFLR_LOCK_ASSERT_HELD(_iflr)                                    \
	LCK_MTX_ASSERT(&(_iflr)->lr_lock, LCK_MTX_ASSERT_OWNED)

#define IFLR_LOCK_ASSERT_NOTHELD(_iflr)                         \
	LCK_MTX_ASSERT(&(_iflr)->lr_lock, LCK_MTX_ASSERT_NOTOWNED)

#define IFLR_LOCK(_iflr)                                                \
	lck_mtx_lock(&(_iflr)->lr_lock)

#define IFLR_LOCK_SPIN(_iflr)                                           \
	lck_mtx_lock_spin(&(_iflr)->lr_lock)

#define IFLR_CONVERT_LOCK(_iflr) do {                                   \
	IFLR_LOCK_ASSERT_HELD(_iflr);                                   \
	lck_mtx_convert_spin(&(_iflr)->lr_lock);                        \
} while (0)

#define IFLR_UNLOCK(_iflr)                                              \
	lck_mtx_unlock(&(_iflr)->lr_lock)

#define IFLR_ADDREF(_iflr)                                              \
	iflr_addref(_iflr, 0)

#define IFLR_ADDREF_LOCKED(_iflr)                                       \
	iflr_addref(_iflr, 1)

#define IFLR_REMREF(_iflr)                                              \
	iflr_remref(_iflr)

struct ifnet_llreach_info;                               

extern void ifnet_llreach_ifattach(struct ifnet *, boolean_t);
extern void ifnet_llreach_ifdetach(struct ifnet *);
extern struct if_llreach *ifnet_llreach_alloc(struct ifnet *, u_int16_t, void *,
    unsigned int, u_int32_t);
extern void ifnet_llreach_free(struct if_llreach *);
extern int ifnet_llreach_reachable(struct if_llreach *);
extern int ifnet_llreach_reachable_delta(struct if_llreach *, u_int64_t);
extern void ifnet_llreach_set_reachable(struct ifnet *, u_int16_t, void *,
    unsigned int);
extern u_int64_t ifnet_llreach_up2calexp(struct if_llreach *, u_int64_t);
extern u_int64_t ifnet_llreach_up2upexp(struct if_llreach *, u_int64_t);
extern int ifnet_llreach_get_defrouter(struct ifnet *, sa_family_t,
    struct ifnet_llreach_info *);
extern void ifnet_lr2ri(struct if_llreach *, struct rt_reach_info *);
extern void ifnet_lr2iflri(struct if_llreach *, struct ifnet_llreach_info *);
extern void ifnet_lr2lri(struct if_llreach *, struct if_llreach_info *);
extern void iflr_addref(struct if_llreach *, int);
extern void iflr_remref(struct if_llreach *);
#endif                         

#ifdef  __cplusplus
}
#endif
#endif              
#endif                          
