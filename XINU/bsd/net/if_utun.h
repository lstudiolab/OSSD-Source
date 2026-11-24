/*
 * Copyright (c) 2008-2019 Apple Inc. All rights reserved.
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


#ifndef _NET_IF_UTUN_H_
#define _NET_IF_UTUN_H_

#ifdef KERNEL_PRIVATE

#include <sys/kern_control.h>

void* utun_alloc(size_t size);
void utun_free(void *ptr);
errno_t utun_register_control(void);

#endif

  
                                             
   
#define UTUN_CONTROL_NAME "com.apple.net.utun_control"

  
                                     
   
#define UTUN_OPT_FLAGS                                  1
#define UTUN_OPT_IFNAME                                 2
#define UTUN_OPT_EXT_IFDATA_STATS                       3                               
#define UTUN_OPT_INC_IFDATA_STATS_IN                    4                                                                          
#define UTUN_OPT_INC_IFDATA_STATS_OUT                   5                                                                          

#define UTUN_OPT_SET_DELEGATE_INTERFACE                 15                                               
#define UTUN_OPT_MAX_PENDING_PACKETS                    16                                                             
                                                                                                
#define UTUN_OPT_ENABLE_CHANNEL                         17
#define UTUN_OPT_GET_CHANNEL_UUID                       18
#define UTUN_OPT_ENABLE_FLOWSWITCH                      19

#define UTUN_OPT_ENABLE_NETIF                           20                                         
#define UTUN_OPT_SLOT_SIZE                              21                                         
#define UTUN_OPT_NETIF_RING_SIZE                        22                                         
#define UTUN_OPT_TX_FSW_RING_SIZE                       23                                         
#define UTUN_OPT_RX_FSW_RING_SIZE                       24                                         
#define UTUN_OPT_KPIPE_TX_RING_SIZE                     25                                         
#define UTUN_OPT_KPIPE_RX_RING_SIZE                     26                                         
#define UTUN_OPT_ATTACH_FLOWSWITCH                      27                                         

  
                              
   
#define UTUN_FLAGS_NO_OUTPUT            0x0001
#define UTUN_FLAGS_NO_INPUT             0x0002
#define UTUN_FLAGS_ENABLE_PROC_UUID     0x0004

  
                                 
   
struct utun_stats_param {
	u_int64_t       utsp_packets;
	u_int64_t       utsp_bytes;
	u_int64_t       utsp_errors;
};

#endif
