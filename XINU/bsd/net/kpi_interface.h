/*
 * Copyright (c) 2004-2020 Apple Inc. All rights reserved.
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
   
                                
                                                                          
                                                                       
                                                                          
   

#ifndef __KPI_INTERFACE__
#define __KPI_INTERFACE__

#ifndef XNU_KERNEL_PRIVATE
#include <TargetConditionals.h>
#endif

#include <sys/kernel_types.h>

#ifdef KERNEL_PRIVATE
struct if_interface_state;
struct ifnet_interface_advisory;
#include <sys/kpi_mbuf.h>
#endif                     

#include <sys/_types/_sa_family_t.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15.4))
#else
#define __NKE_API_DEPRECATED
#endif              

#ifdef XNU_KERNEL_PRIVATE
#if !XNU_TARGET_OS_OSX || (TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
#define KPI_INTERFACE_EMBEDDED 1
#else                                                                       
#define KPI_INTERFACE_EMBEDDED 0
#endif                                                                       
#else
#if (TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
#define KPI_INTERFACE_EMBEDDED 1
#else
#define KPI_INTERFACE_EMBEDDED 0
#endif
#endif

struct timeval;
struct sockaddr;
struct sockaddr_dl;
struct kern_event_msg;
struct kev_msg;
struct ifnet_demux_desc;

   
                                 
                                                         
                                                                       
                                                                       
                                                               
                                                      
                                                       
                                                             
                                                    
                                                    
                                                      
                                                          
                                                               
                                                                                
                                                     
                                                                           
                                                                
                                                              
                                                            
                                                      
                                                         
   
enum {
	IFNET_FAMILY_ANY                = 0,
	IFNET_FAMILY_LOOPBACK           = 1,
	IFNET_FAMILY_ETHERNET           = 2,
	IFNET_FAMILY_SLIP               = 3,
	IFNET_FAMILY_TUN                = 4,
	IFNET_FAMILY_VLAN               = 5,
	IFNET_FAMILY_PPP                = 6,
	IFNET_FAMILY_PVC                = 7,
	IFNET_FAMILY_DISC               = 8,
	IFNET_FAMILY_MDECAP             = 9,
	IFNET_FAMILY_GIF                = 10,
	IFNET_FAMILY_FAITH              = 11,                   
	IFNET_FAMILY_STF                = 12,
	IFNET_FAMILY_FIREWIRE           = 13,
	IFNET_FAMILY_BOND               = 14,
	IFNET_FAMILY_CELLULAR           = 15,
	IFNET_FAMILY_6LOWPAN            = 16,
	IFNET_FAMILY_UTUN               = 17,
	IFNET_FAMILY_IPSEC              = 18
};

   
                                
                                                         
   
typedef u_int32_t ifnet_family_t;

#ifdef KERNEL_PRIVATE
  
                                     
                                                                      
                                                                       
                                                                    
                                                                  
                                                                     
   
enum {
	IFNET_SUBFAMILY_ANY             = 0,
	IFNET_SUBFAMILY_USB             = 1,
	IFNET_SUBFAMILY_BLUETOOTH       = 2,
	IFNET_SUBFAMILY_WIFI            = 3,
	IFNET_SUBFAMILY_THUNDERBOLT     = 4,
	IFNET_SUBFAMILY_RESERVED        = 5,
	IFNET_SUBFAMILY_INTCOPROC       = 6,
	IFNET_SUBFAMILY_QUICKRELAY      = 7,
	IFNET_SUBFAMILY_DEFAULT         = 8,
	IFNET_SUBFAMILY_VMNET           = 9,
};

  
                                    
                                                             
   
typedef u_int32_t ifnet_subfamily_t;
#endif                     

#ifndef BPF_TAP_MODE_T
#define BPF_TAP_MODE_T
   
                           
                                                         
                                                 
                                                    
                                                      
                                                                 
   

enum {
	BPF_MODE_DISABLED               = 0,
	BPF_MODE_INPUT                  = 1,
	BPF_MODE_OUTPUT                 = 2,
	BPF_MODE_INPUT_OUTPUT           = 3
};
   
                              
                                                                                 
   
typedef u_int32_t bpf_tap_mode;
#endif                      

   
                                   
                                                        
   
typedef u_int32_t protocol_family_t;

   
                                  
                                                                
                                                                        
                                                                        
                                                                        
                                                                           
                                                                       
                                                                                 
                                                                                 
                                                                           
                                                                          
                                                             
                                                                         
                                                                             
                                                                           
                                                                          
                                                                       
                                                                        
                                                                            
                                                                             
                                                                                
                                                                        
                                                                       
                                                                        
                                                                                
                                                                                                        
                                                                                                         
                                                                                                         
                                                                                                         
                                                                                                        
                                                                                
                                                                                                             
                                                                                                         
                                                                                                         
                                                                                                         
                                                                                                             
                                                                         
                                                                            
                                                                            
                                                                  
  
   

enum {
	IFNET_CSUM_IP           = 0x00000001,
	IFNET_CSUM_TCP          = 0x00000002,
	IFNET_CSUM_UDP          = 0x00000004,
	IFNET_CSUM_FRAGMENT     = 0x00000008,
	IFNET_IP_FRAGMENT       = 0x00000010,
	IFNET_CSUM_TCPIPV6      = 0x00000020,
	IFNET_CSUM_UDPIPV6      = 0x00000040,
	IFNET_IPV6_FRAGMENT     = 0x00000080,
#ifdef KERNEL_PRIVATE
	IFNET_CSUM_PARTIAL      = 0x00001000,
	IFNET_CSUM_SUM16        = IFNET_CSUM_PARTIAL,
	IFNET_CSUM_ZERO_INVERT  = 0x00002000,
#endif                     
	IFNET_VLAN_TAGGING      = 0x00010000,
	IFNET_VLAN_MTU          = 0x00020000,
	IFNET_MULTIPAGES        = 0x00100000,
	IFNET_TSO_IPV4          = 0x00200000,
	IFNET_TSO_IPV6          = 0x00400000,
	IFNET_TX_STATUS         = 0x00800000,
	IFNET_HW_TIMESTAMP      = 0x01000000,
	IFNET_SW_TIMESTAMP      = 0x02000000
};
   
                                 
                                                                         
   
typedef u_int32_t ifnet_offload_t;

#ifdef KERNEL_PRIVATE
#define IFNET_OFFLOADF_BITS \
	"\020\1CSUM_IP\2CSUM_TCP\3CSUM_UDP\4CSUM_IP_FRAGS\5IP_FRAGMENT" \
	"\6CSUM_TCPIPV6\7CSUM_UDPIPV6\10IPV6_FRAGMENT\15CSUM_PARTIAL"   \
	"\16CSUM_ZERO_INVERT\20VLAN_TAGGING\21VLAN_MTU\25MULTIPAGES"    \
	"\26TSO_IPV4\27TSO_IPV6\30TXSTATUS\31HW_TIMESTAMP\32SW_TIMESTAMP"

#define IFNET_CHECKSUMF                                                 \
	(IFNET_CSUM_IP | IFNET_CSUM_TCP | IFNET_CSUM_UDP |              \
	IFNET_CSUM_FRAGMENT | IFNET_CSUM_TCPIPV6 | IFNET_CSUM_UDPIPV6 | \
	IFNET_CSUM_PARTIAL | IFNET_CSUM_ZERO_INVERT)

#define IFNET_TSOF                                                      \
	(IFNET_TSO_IPV4	| IFNET_TSO_IPV6)
#endif                     

  
            
  
                                                                         
   
   
                                 
  
                                                                             
                                                                               
                                                                                 
                                                             
                                                                  
                                                              
                                                     
   
                                                                 
typedef errno_t (*bpf_packet_func)(ifnet_t interface, mbuf_t data);

   
                                   
  
                                                                             
                                                                               
                                                                      
                                                  
                                                      
                                           
   
                                                                 
typedef errno_t (*ifnet_output_func)(ifnet_t interface, mbuf_t data);

   
                                  
                                                                            
                                     
  
                                                                              
                                                                              
                                               
                                                                   
                                      
                                                                
   
typedef errno_t (*ifnet_ioctl_func)(ifnet_t interface, unsigned long cmd,
    void *data);

   
                                   
                                                                          
                                 
   
typedef errno_t (*ifnet_set_bpf_tap)(ifnet_t interface, bpf_tap_mode mode,
    bpf_packet_func callback);

   
                                     
                                                                           
                                                                                 
                                                                                
                                                                                
                                                                          
                                                                         
                                                                                
                                                                           
                                                                               
                              
                                                               
                       
   
typedef void (*ifnet_detached_func)(ifnet_t interface);

   
                                  
                                                                          
                                                                            
                                                                         
                                                                          
                                                                              
                                                                            
                                          
                                                                   
                                                      
                                                           
                                                                             
                                              
                
                                                                          
                                                                           
                                                  
                                                                             
                                                  
   
typedef errno_t (*ifnet_demux_func)(ifnet_t interface, mbuf_t packet,
    char *frame_header, protocol_family_t *protocol_family);

   
                                  
                                                                         
                                    
                                                              
   
typedef void (*ifnet_event_func)(ifnet_t interface, const struct kev_msg *msg);

   
                                   
                                                                            
                                                                                
                         
                                                                    
                                                                            
                                                                                 
                                                                                
                                                  
                                                                             
                                                                         
                                                
                                                                         
                                     
                                                                           
                                                                      
                                                                                 
                                                                      
                
                                                                          
                                                                           
                                                  
                                                                             
                                                  
   
typedef errno_t (*ifnet_framer_func)(ifnet_t interface, mbuf_t *packet,
    const struct sockaddr *dest, const char *dest_linkaddr,
    const char *frame_type
#if KPI_INTERFACE_EMBEDDED
    , u_int32_t *prepend_len, u_int32_t *postpend_len
#endif                             
    );
#ifdef KERNEL_PRIVATE
typedef errno_t (*ifnet_framer_extended_func)(ifnet_t interface, mbuf_t *packet,
    const struct sockaddr *dest, const char *dest_linkaddr,
    const char *frame_type, u_int32_t *prepend_len,
    u_int32_t *postpend_len);
#endif                     

   
                                      
                                                                             
                                                                         
                                                                             
                                                                           
                                                                         
                                                                          
                                                                       
                                                                             
                                         
                                                                         
                
                                                                          
                                                                         
                                 
   
typedef errno_t (*ifnet_add_proto_func)(ifnet_t interface,
    protocol_family_t protocol_family,
    const struct ifnet_demux_desc *demux_array, u_int32_t demux_count);

   
                                   
                                                                             
                                                                                 
                                                                         
                                                           
                                                                           
                                                                          
                
                                                                          
                                                                         
                                                                      
   
typedef errno_t (*ifnet_del_proto_func)(ifnet_t interface,
    protocol_family_t protocol_family);

   
                                   
                                                                           
                                                                               
                                                                               
                                  
  
                                                                               
                                                                               
                                                
                                        
                                            
                
                                                                       
                                                                            
   
typedef errno_t (*ifnet_check_multi)(ifnet_t interface,
    const struct sockaddr *mcast);

   
                                   
                                                                            
                                                                              
                                                                        
                                                             
                                                             
                                              
                                        
                
                                                                             
                                                
                                                                               
                                         
   
typedef errno_t (*proto_media_input)(ifnet_t ifp, protocol_family_t protocol,
    mbuf_t packet, char *header);

   
                                      
                                                                               
                                                                              
                                                                          
                                                                               
                                                                              
                                                                           
                                    
                                                             
                                                             
                                              
                
                                                                               
                                                
                                                                               
                                          
   
typedef errno_t (*proto_media_input_v2)(ifnet_t ifp, protocol_family_t protocol,
    mbuf_t packet);

   
                                    
                                                                        
                                                                              
                                                                         
                             
                                                             
                                                              
                                  
                                             
                                                            
                                                                        
                                                         
                                                               
                
                                                                                 
                                                                           
                                                                              
                        
   
typedef errno_t (*proto_media_preout)(ifnet_t ifp, protocol_family_t protocol,
    mbuf_t *packet, const struct sockaddr *dest, void *route, char *frame_type,
    char *link_layer_dest);

   
                                   
                                                                        
                                           
                                  
                                             
                                
   
typedef void (*proto_media_event)(ifnet_t ifp, protocol_family_t protocol,
    const struct kev_msg *event);

   
                                   
                                                                          
                                                                                 
                                                                                 
                                                                              
                                                                       
                                                                                
                                                                                 
                                                                          
  
                                                                              
                                                                              
                                               
                                  
                                             
                                          
                                                   
                
                                    
   
typedef errno_t (*proto_media_ioctl)(ifnet_t ifp, protocol_family_t protocol,
    unsigned long command, void *argument);

   
                                      
                                                                         
                                   
                                  
                                             
                
                                    
   
typedef errno_t (*proto_media_detached)(ifnet_t ifp, protocol_family_t protocol);

   
                                           
                                                                     
                                                                           
                         
                                  
                                                
                                                                            
                                                               
                                                                           
   
typedef errno_t (*proto_media_resolve_multi)(ifnet_t ifp,
    const struct sockaddr *proto_addr, struct sockaddr_dl *out_ll,
    size_t ll_len);

   
                                      
                                                                            
                                                                               
                                                                           
                                                        
                                                                   
                                                                 
                              
                                                                  
                                                                         
                                  
                                                                     
                                                      
                                                                           
                                                                               
                                                                           
                                                                                 
                                                                                 
                         
                                                                          
                      
                                                                           
   
typedef errno_t (*proto_media_send_arp)(ifnet_t ifp, u_short arpop,
    const struct sockaddr_dl *sender_hw, const struct sockaddr *sender_proto,
    const struct sockaddr_dl *target_hw, const struct sockaddr *target_proto);

   
                                           
                                                                       
                                   
                                                          
                                                      
                                                       
                                                              
                                                          
                                                             
                                                                           
                                                      
   
struct ifnet_stat_increment_param {
	u_int32_t                               packets_in;
	u_int32_t                               bytes_in;
	u_int32_t                               errors_in;

	u_int32_t                               packets_out;
	u_int32_t                               bytes_out;
	u_int32_t                               errors_out;

	u_int32_t                               collisions;
	u_int32_t                               dropped;
};

   
                                  
                                                                           
                                                                           
                                                                          
                                                 
                                                                     
                           
                                                                   
                                                  
                                                                        
                                            
                                                                          
                                                        
                                                                              
                                                                          
                                       
                                                                              
                                 
                                                                        
                           
                                                                          
                           
                                                                                
                                                                               
                                                          
                                                         
                                                                          
                                                                               
                                   
                                                                               
                                        
                                                                        
                           
                                                                             
   
struct ifnet_init_params {
	                                      
	const void              *uniqueid;                            
	u_int32_t               uniqueid_len;                         

	                                                   
	const char              *name;                                
	u_int32_t               unit;                                 
	ifnet_family_t          family;                               
	u_int32_t               type;                                 
	ifnet_output_func       output;                               
	ifnet_demux_func        demux;                                 
	ifnet_add_proto_func    add_proto;                             
	ifnet_del_proto_func    del_proto;                             
	ifnet_check_multi       check_multi;                                                            
	ifnet_framer_func       framer;                               
	void                    *softc;                               
	ifnet_ioctl_func        ioctl;                                
	ifnet_set_bpf_tap       set_bpf_tap;                            
	ifnet_detached_func     detach;                               
	ifnet_event_func        event;                                
	const void              *broadcast_addr;                                                        
	u_int32_t               broadcast_len;                                                          
};

#ifdef KERNEL_PRIVATE
                              
#define IFNET_INIT_VERSION_2            2
#define IFNET_INIT_CURRENT_VERSION      IFNET_INIT_VERSION_2

                            
#define IFNET_INIT_LEGACY       0x1                                         
#define IFNET_INIT_INPUT_POLL   0x2                                            
#define IFNET_INIT_NX_NOAUTO    0x4                                   
#define IFNET_INIT_ALLOC_KPI    0x8                                              

  
                                        
                                                                              
                                                                              
                                                                                
                                                                       
                                                      
                                           
   
typedef errno_t (*ifnet_pre_enqueue_func)(ifnet_t interface, mbuf_t data);

  
                                  
                                                                            
                                                                               
                                                                         
                                                                        
                                                                          
                                                                  
                                                                         
                                                                      
                                                                        
                                                                         
                                                                    
                                                                       
                                     
                                                      
   
typedef void (*ifnet_start_func)(ifnet_t interface);

  
                                       
                                                                            
                                                                              
                                            
                                                                     
                                     
                                                                       
                                                                        
                                                                      
                                                                            
                                                                        
                                         
   
typedef void (*ifnet_input_poll_func)(ifnet_t interface, u_int32_t flags,
    u_int32_t max_count, mbuf_t *first_packet, mbuf_t *last_packet,
    u_int32_t *cnt, u_int32_t *len);

   
                                 
                                                                                     
                                                                                       
                                                      
                                                            
   
typedef void (*ifnet_free_func)(ifnet_t interface);

  
                                         
                                                       
                    
                                                             
                                                             
                                                       
                                                       
   
enum {
	IFNET_CTL_SET_INPUT_MODEL       = 1,                   
	IFNET_CTL_GET_INPUT_MODEL       = 2,                   
	IFNET_CTL_SET_LOG               = 3,                    
	IFNET_CTL_GET_LOG               = 4,                    
	IFNET_CTL_NOTIFY_ADDRESS        = 5                     
};

  
                                 
                                                                  
   
typedef u_int32_t ifnet_ctl_cmd_t;

  
                                           
                                                         
                    
                                                                             
                                                                                
                                                                            
                                                                             
                                                                             
                                                                             
                                                                         
                                             
                                                                              
                                                                              
                                                                             
                                                                              
                                                                              
                                                                              
                                                                         
                                          
   
enum {
	IFNET_MODEL_INPUT_POLL_OFF      = 0,
	IFNET_MODEL_INPUT_POLL_ON       = 1,
};

  
                               
                                                                    
   
typedef u_int32_t ifnet_model_t;

  
                                   
                                                                           
                              
                                          
   
struct ifnet_model_params {
	ifnet_model_t           model;
	u_int32_t               reserved[3];
};

  
                                              
                                                                         
                                                                         
                                                          
                    
                                                                     
                                                                              
                                                              
                                                        
                                                            
                                                                          
                                                                      
                                                            
   
enum {
	IFNET_LOG_DEFAULT       = 0,
	IFNET_LOG_ALERT         = 1,
	IFNET_LOG_CRITICAL      = 2,
	IFNET_LOG_ERROR         = 3,
	IFNET_LOG_WARNING       = 4,
	IFNET_LOG_NOTICE        = 5,
	IFNET_LOG_INFORMATIONAL = 6,
	IFNET_LOG_DEBUG         = 7
};

#ifdef BSD_KERNEL_PRIVATE
#define IFNET_LOG_MIN   IFNET_LOG_DEFAULT
#define IFNET_LOG_MAX   IFNET_LOG_DEBUG
#endif                         

  
                                   
                                                       
   
typedef int32_t ifnet_log_level_t;

  
                                           
                                                                  
                                                                       
                    
                                                  
                                                                 
                                                             
                                                          
   
enum {
	IFNET_LOGF_DLIL         = 0x00000001,
	IFNET_LOGF_FAMILY       = 0x00010000,
	IFNET_LOGF_DRIVER       = 0x01000000,
	IFNET_LOGF_FIRMWARE     = 0x10000000
};

#ifdef BSD_KERNEL_PRIVATE
#define IFNET_LOGF_MASK                                                 \
	(IFNET_LOGF_DLIL | IFNET_LOGF_FAMILY | IFNET_LOGF_DRIVER |      \
	IFNET_LOGF_FIRMWARE)

#define IFNET_LOGF_BITS \
	"\020\1DLIL\21FAMILY\31DRIVER\35FIRMWARE"

#endif                         

  
                                   
                                                         
   
typedef u_int32_t ifnet_log_flags_t;

  
                                         
                                                                         
                    
                                                                         
                                                                        
                                                                       
   
enum {
	IFNET_LOGCAT_CONNECTIVITY       = 1,
	IFNET_LOGCAT_QUALITY            = 2,
	IFNET_LOGCAT_PERFORMANCE        = 3
};

  
                                      
                                                 
   
typedef int32_t ifnet_log_category_t;

  
                                         
                                                                            
                                                             
   
typedef int32_t ifnet_log_subcategory_t;

  
                                 
                                                                     
                                      
                                                 
                                                   
                                               
                                                     
   
struct ifnet_log_params {
	ifnet_log_level_t       level;
	ifnet_log_flags_t       flags;
	ifnet_log_category_t    category;
	ifnet_log_subcategory_t subcategory;
};

  
                                            
                                                                     
                                                                            
                                                                             
                                                                           
                                                                             
                                                                                 
                                                                               
   
struct ifnet_notify_address_params {
	sa_family_t             address_family;
	u_int32_t               reserved[3];
};

  
                                
                                                                            
                                                                        
                                                                           
                                        
                                                                  
                                                          
                                         
                                                       
   
typedef errno_t (*ifnet_ctl_func)(ifnet_t interface, ifnet_ctl_cmd_t cmd,
    u_int32_t arglen, void *arg);

  
                                   
                                                                           
                                                                              
                                                                                
                                                 
                                                                              
                                                 
                                                 
                                                                     
                           
                                                                   
                                                  
                                                                        
                                            
                                                                          
                                                        
                                                                            
                                                 
                                                                              
                                                                          
                                       
                                                                             
                                                                                 
                                                                              
                                                      
                                                                               
                                                      
                                                                             
                                              
                                                                       
                                                                   
                                              
                                                                              
                                                                      
                                      
                                                                       
                                                                    
                                 
                                                                       
                                                                  
                                                            
                                                                        
                                                                          
                                                                        
                                                                   
                                        
                                                                             
                                                                               
                        
                                                                             
                                                                               
                                                                         
                                                                              
                                                                             
                                              
                                                                            
                                                                    
                                      
                                                                     
                                                                  
                                 
                                                                              
                                 
                                                                        
                           
                                                                          
                           
                                                                                
                                                                            
                                                                                
                                          
                                                                               
                                                          
                                                         
                                                                          
                                                                               
                                   
                                                                               
                                        
                                                                        
                           
                                                                             
                                                                              
                                                                               
                                               
                                                                            
                                                                               
                                                                            
                                           
   
struct ifnet_init_eparams {
	u_int32_t               ver;                                  
	u_int32_t               len;                                  
	u_int32_t               flags;                                

	                                      
	const void              *uniqueid;                            
	u_int32_t               uniqueid_len;                         

	                                                   
	const char              *name;                                
	u_int32_t               unit;                                 
	ifnet_family_t          family;                               
	u_int32_t               type;                                 
	u_int32_t               sndq_maxlen;                                              
	ifnet_output_func       output;                                                     
	ifnet_pre_enqueue_func  pre_enqueue;                                              
	ifnet_start_func        start;                                                   
	ifnet_ctl_func          output_ctl;                                               
	u_int32_t               output_sched_model;                                       
	u_int32_t               output_target_qdelay;                                                  
	u_int64_t               output_bw;                            
	u_int64_t               output_bw_max;                        
	u_int64_t               output_lt;                            
	u_int64_t               output_lt_max;                        
	u_int16_t               start_delay_qlen;                     
	u_int16_t               start_delay_timeout;                  
	u_int32_t               _reserved[3];                               
	ifnet_input_poll_func   input_poll;                                                     
	ifnet_ctl_func          input_ctl;                                                      
	u_int32_t               rcvq_maxlen;                                                          
	u_int32_t               __reserved;                                 
	u_int64_t               input_bw;                             
	u_int64_t               input_bw_max;                         
	u_int64_t               input_lt;                             
	u_int64_t               input_lt_max;                         
	u_int64_t               ___reserved[2];                             
	ifnet_demux_func        demux;                                 
	ifnet_add_proto_func    add_proto;                             
	ifnet_del_proto_func    del_proto;                             
	ifnet_check_multi       check_multi;                                                            
	ifnet_framer_func       framer;                               
	void                    *softc;                               
	ifnet_ioctl_func        ioctl;                                
	ifnet_set_bpf_tap       set_bpf_tap;                            
	ifnet_detached_func     detach;                               
	ifnet_event_func        event;                                
	const void              *broadcast_addr;                                                        
	u_int32_t               broadcast_len;                                                          
	ifnet_framer_extended_func framer_extended;                   
	ifnet_subfamily_t       subfamily;                            
	u_int16_t               tx_headroom;                          
	u_int16_t               tx_trailer;                           
	u_int32_t               rx_mit_ival;                          
#if !defined(__LP64__)
	ifnet_free_func         free;                                 
	u_int32_t               _____reserved;                              
	u_int64_t               ____reserved[1];                            
#else
	u_int32_t               ____reserved;                               
	ifnet_free_func         free;                                 
#endif               
};
#endif                     

   
                                  
                                                                     
                            
                                                          
                                                      
                                                       
                                                              
                                                          
                                                             
                                                                           
                                                      
   
struct ifnet_stats_param {
	u_int64_t       packets_in;
	u_int64_t       bytes_in;
	u_int64_t       multicasts_in;
	u_int64_t       errors_in;

	u_int64_t       packets_out;
	u_int64_t       bytes_out;
	u_int64_t       multicasts_out;
	u_int64_t       errors_out;

	u_int64_t       collisions;
	u_int64_t       dropped;
	u_int64_t       no_protocol;
};

   
                                 
                                                                           
                                                                               
                                                                         
                                                                                 
                                                                             
                                                                              
                                                                                
                                                            
                                                                         
                                                                            
                                                                        
                        
   
struct ifnet_demux_desc {
	u_int32_t       type;
	void            *data;
	u_int32_t       datalen;
};

   
                                         
                                                                      
                                                                             
                                                                                
                                                                                
                                                                   
                                         
                                                                           
                                                                    
                                                                          
                                                                     
                                                           
                                                                           
   
#ifdef KERNEL_PRIVATE
#define demux_list demux_array
#endif                     

struct ifnet_attach_proto_param {
	struct ifnet_demux_desc         *demux_array;                                      
	u_int32_t                       demux_count;                                       

	proto_media_input               input;                        
	proto_media_preout              pre_output;                   
	proto_media_event               event;                        
	proto_media_ioctl               ioctl;                        
	proto_media_detached            detached;                     
	proto_media_resolve_multi       resolve;                      
	proto_media_send_arp            send_arp;                     
};

struct ifnet_attach_proto_param_v2 {
	struct ifnet_demux_desc         *demux_array;                                      
	u_int32_t                       demux_count;                                       

	proto_media_input_v2            input;                        
	proto_media_preout              pre_output;                   
	proto_media_event               event;                        
	proto_media_ioctl               ioctl;                        
	proto_media_detached            detached;                     
	proto_media_resolve_multi       resolve;                      
	proto_media_send_arp            send_arp;                     
};

__BEGIN_DECLS

  
                                        
   

   
                                 
                                                                            
                                                                            
                                                                                 
                                                                               
                                                                             
                                                                          
                                                                                 
                                                                           
                                                                                
                                                                                 
                                                                       
                                                                                
                                        
                                                                           
                                                                       
                                                               
                                                                            
                                                                              
                                              
   
#ifdef KERNEL_PRIVATE
extern errno_t ifnet_allocate_internal(const struct ifnet_init_params *init,
    ifnet_t *interface);

#define ifnet_allocate(init, interface) \
	ifnet_allocate_internal((init), (interface))
#else
extern errno_t ifnet_allocate(const struct ifnet_init_params *init,
    ifnet_t *interface)
__NKE_API_DEPRECATED;
#endif                     

#ifdef KERNEL_PRIVATE
  
                                          
                                                                                 
                                                                    
                                                                           
                                                                       
                                                               
                                                                           
                                                                                            
                                              
   
extern errno_t ifnet_allocate_extended(const struct ifnet_init_eparams *init,
    ifnet_t *interface);

  
                                
                                                                          
                                                             
                                                  
   
extern void ifnet_dispose(ifnet_t interface);

  
                              
                                                                            
                                             
                                                 
   
extern void ifnet_purge(ifnet_t interface);

  
                                
                                                                         
                                                              
                                                                 
                                                                            
                                          
                                                                          
                                                                             
                                                                              
                                                                             
                                                                           
                                                                            
                                                                                
   
extern errno_t ifnet_enqueue(ifnet_t interface, mbuf_t packet);

  
                                
                                                                           
                                                                           
                                                                            
                                                                   
                                                            
                                                                          
                                                                            
                                                                               
                                                                       
                             
   
extern errno_t ifnet_dequeue(ifnet_t interface, mbuf_t *packet);

  
                                              
                                                                            
                                                                              
                                                                              
                                                            
                                                                   
                                     
                                                            
                                                                          
                                                                            
                                                                      
                                                                     
                                                                 
   
extern errno_t ifnet_dequeue_service_class(ifnet_t interface,
    mbuf_svc_class_t sc, mbuf_t *packet);

  
                                      
                                                                            
                                                                                 
                                                                                
                                                                              
                                                                    
                                                                          
                                                                           
                                                     
                                                                        
                                                                              
                                                            
                                                                            
                                                                   
                                                                        
                                                                        
                                     
                                                                          
                                                                            
                                                                               
                                                                       
                             
   
extern errno_t ifnet_dequeue_multi(ifnet_t interface, u_int32_t max,
    mbuf_t *first_packet, mbuf_t *last_packet, u_int32_t *cnt, u_int32_t *len);

  
                                            
                                                                         
                                                                           
                                                     
                                                                          
                                                                          
                                                                       
                                                    
                                                                   
                                                                         
                                                                       
                                                              
                                                                       
                                                                     
                                                                          
                                                                       
                                                                        
                                                                       
                                          
                                                                          
                                                                      
                                                           
                                                                          
                                                   
   
extern errno_t ifnet_dequeue_multi_bytes(ifnet_t interface,
    u_int32_t max_bytes, mbuf_t *first_packet, mbuf_t *last_packet,
    u_int32_t *cnt, u_int32_t *len);

  
                                                    
                                                                              
                                                                        
                                                                            
                                                                             
                                                                              
                                                                    
                                     
                                                                          
                                                                           
                                                     
                                                                        
                                                                              
                                                            
                                                                            
                                                                   
                                                                        
                                                                        
                                     
                                                                          
                                                                            
                                                                      
                                                                     
                                                                 
   
extern errno_t ifnet_dequeue_service_class_multi(ifnet_t interface,
    mbuf_svc_class_t sc, u_int32_t max, mbuf_t *first_packet,
    mbuf_t *last_packet, u_int32_t *cnt, u_int32_t *len);

  
                                               
                                                                          
                                                        
                                                                   
                                                                        
                                                                            
                                                                             
   
extern errno_t ifnet_set_output_sched_model(ifnet_t interface,
    u_int32_t model);

  
                                        
                                                                     
                                                                        
                                                                        
                                                                           
                                          
                                                                       
                                                                          
                                                                            
                                                                             
   
extern errno_t ifnet_set_sndq_maxlen(ifnet_t interface, u_int32_t maxqlen);

  
                                        
                                                                     
                                                                        
                                                                       
                                                                              
                                                             
                                                                            
                                                                             
   
extern errno_t ifnet_get_sndq_maxlen(ifnet_t interface, u_int32_t *maxqlen);

  
                                     
                                                                     
                                                                        
                                                                           
                                                                              
                                                                           
                                                                      
                                                                            
                                                                             
   
extern errno_t ifnet_get_sndq_len(ifnet_t interface, u_int32_t *packets);

  
                                                   
                                                                              
                                                                              
                              
                                                                           
                                     
                                                                              
                                                                          
                                                                             
                                                                                
                                                                            
                                                                             
                                                                               
                                                                          
                                                                           
                                                                      
                                                                            
                                                                             
   
extern errno_t ifnet_get_service_class_sndq_len(ifnet_t interface,
    mbuf_svc_class_t sc, u_int32_t *packets, u_int32_t *bytes);

  
                                        
                                                                    
                                                                       
                                                                        
                                                                          
                                          
                                                                       
                                                                         
                                                                           
                                                                            
                                                                            
                                                                            
   
extern errno_t ifnet_set_rcvq_maxlen(ifnet_t interface, u_int32_t maxqlen);

  
                                        
                                                                    
                                                                       
                                                                       
                                                                              
                                    
                                                                            
                                                                            
   
extern errno_t ifnet_get_rcvq_maxlen(ifnet_t interface, u_int32_t *maxqlen);

  
                                  
                                                                           
                                                     
                                                                    
                                                                          
                                                                            
                                                                             
                                                                             
                                                               
                                                             
                                                              
                                                           
                                                            
                                                                                
                                                                             
                                                                         
                                                                                
                                                                             
                                                                           
                                                                          
                                                                           
                                                                            
                                                          
                                                                          
                                                                          
                                                                         
                                                                             
                                                                           
                                      
   
struct ifnet_poll_params {
	u_int32_t       flags;
	u_int32_t       packets_limit;
	u_int32_t       packets_lowat;
	u_int32_t       packets_hiwat;
	u_int32_t       bytes_lowat;
	u_int32_t       bytes_hiwat;
	u_int64_t       interval_time;
	u_int64_t       reserved[4];
};

typedef struct ifnet_poll_params ifnet_poll_params_t;

  
                                        
                                                                            
                                                                               
                                                                              
                                                                             
                                                                              
                                                                              
                                                                           
                                                                              
                                                   
                                                                            
                                                                            
   
extern errno_t ifnet_set_poll_params(ifnet_t interface,
    ifnet_poll_params_t *poll_params);

  
                                    
                                                                           
                                                                               
                                                                
                                                                              
                                                              
                                                                            
                                                                            
   
extern errno_t ifnet_poll_params(ifnet_t interface,
    ifnet_poll_params_t *poll_params);

  
                              
                                                                       
                                                                        
                                                                     
   
extern void ifnet_start(ifnet_t interface);

  
                               
                                                                           
                                                                          
                                                                          
                                                              
                                                                     
                                                    
                                                                            
                                                                            
   
extern errno_t ifnet_flowid(ifnet_t interface, u_int32_t *flowid);

  
                                      
                                                                           
                                                                           
                                                                          
                                                                              
                                                                            
                                                                              
                                                                             
                                                                              
                                                                             
                                                                      
                                                                            
                                                                            
   
extern errno_t ifnet_enable_output(ifnet_t interface);

  
                                       
                                                                         
                                                                           
                                                                        
                                                                         
                                                                          
                                                                          
                                                                              
                                                                          
                                                                          
                                                                       
                                                                        
                                                                       
                                                                            
                                                                            
   
extern errno_t ifnet_disable_output(ifnet_t interface);
#endif                     

   
                                  
                                                                         
                                                                             
                                     
                                                                            
                                                                 
   
extern errno_t ifnet_reference(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                
                                                                         
                                                       
                                                                           
                                   
                                                                 
   
extern errno_t ifnet_release(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                               
                                                                            
                                                                            
                                                                               
                                        
  
                                                                             
                                                                              
                     
                                                  
                                                                             
                                                                              
                                                                                 
                                                     
                                                                         
                           
   
extern errno_t ifnet_attach(ifnet_t interface,
    const struct sockaddr_dl *ll_addr)
__NKE_API_DEPRECATED;

   
                               
                                            
  
                                                                                 
                                                                           
                                                                     
  
                                                                               
                                                                              
                                                                          
                                                                             
                                                                         
                                                                      
  
                                                                      
                                                                    
  
                                                                             
                                                                              
                     
                                                  
                                                     
   
extern errno_t ifnet_detach(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                              
                                                                          
                                                                                    
                                                                           
                                                                                 
                                                      
                                                                                   
                                                                                        
                                                                                           
                                    
                                                                                
                                                                                        
                                                                             
                             
                                                     
   
extern errno_t ifnet_interface_family_find(const char *module_string, ifnet_family_t *family_id)
__NKE_API_DEPRECATED;

  
                          
   

   
                              
                                                                           
                                                                 
                                          
   
extern void *ifnet_softc(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                             
                                                                    
                                                              
                                     
   
extern const char *ifnet_name(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                               
                                                         
                                                                
                                       
   
extern ifnet_family_t ifnet_family(ifnet_t interface)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                  
                                                             
                                                                    
                                           
   
extern ifnet_subfamily_t ifnet_subfamily(ifnet_t interface);
#endif                     

   
                             
                                                              
                                                                     
                             
   
extern u_int32_t ifnet_unit(ifnet_t interface)
__NKE_API_DEPRECATED;


   
                              
                                                                         
                                                                              
                                                                                 
                                                                          
                                    
                                                             
                       
   
extern u_int32_t ifnet_index(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                  
                                                                 
                                                                         
                                                                                
                                                                        
                                                                              
                                                        
                                                                        
                                              
                                                        
   
extern errno_t ifnet_set_flags(ifnet_t interface, u_int16_t new_flags,
    u_int16_t mask)
__NKE_API_DEPRECATED;

   
                              
                                                              
                                                               
                                                           
   
extern u_int16_t ifnet_flags(ifnet_t interface)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                   
                                                                         
                                                                               
                                                                              
                                                                        
                                    
                                        
                                                                        
                                              
                                                      
                                                        
   
extern errno_t ifnet_set_eflags(ifnet_t interface, u_int32_t new_flags,
    u_int32_t mask);

  
                               
                                                                       
                                                               
                                                                    
   
extern u_int32_t ifnet_eflags(ifnet_t interface);

  
                                       
                                                                       
                                                                          
                                                                          
                                               
                                                                      
                                                                       
                                                                        
                                            
                                        
                                                                        
                                              
                                                      
                                                                             
                                                                    
   
extern errno_t ifnet_set_idle_flags(ifnet_t interface, u_int32_t new_flags,
    u_int32_t mask);

  
                                   
                                                        
                                                               
                                                                   
   
extern u_int32_t ifnet_idle_flags(ifnet_t interface);

  
                                         
                                                                
                                                                            
                                  
                                                               
                                                                           
                                                                               
   
extern errno_t ifnet_set_link_quality(ifnet_t interface, int quality);

  
                                     
                                                                   
                                                               
                                                 
   
extern int ifnet_link_quality(ifnet_t interface);

  
                                            
                                                            
                                                                        
                           
                                                              
                                                                           
                                                                               
   
extern errno_t ifnet_set_interface_state(ifnet_t interface,
    struct if_interface_state *if_interface_state);

  
                                            
                                                               
                                          
                                              
   
extern int ifnet_get_interface_state(ifnet_t interface,
    struct if_interface_state *if_interface_state);

  
                                   
                                                                      
                                                             
                                                                          
                                                        
                                                                    
                                                                              
                                                                              
                                                                              
                                                                             
                                                                           
                                                                           
                                                                        
                                            
                                                                       
                                                                               
                                                                             
                                                                             
                                                                              
                                                                            
                                                                               
                                                 
                                                                              
                                                                               
                                                              
                                                                                
                                                                                
                                                                              
                                                                         
                                                                            
                                                                               
                                                                         
                                                           
                                                                           
                                                                            
                                 
   
#define IFNET_LLREACHINFO_ADDRLEN       64                           

struct ifnet_llreach_info {
	u_int32_t iflri_refcnt;
	u_int32_t iflri_probes;
	u_int64_t iflri_snd_expire;
	u_int64_t iflri_rcv_expire;
	u_int64_t iflri_curtime;
	u_int32_t iflri_netproto;
	u_int8_t  iflri_addr[IFNET_LLREACHINFO_ADDRLEN];
	int32_t   iflri_rssi;
	int32_t   iflri_lqm;
	int32_t   iflri_npm;
};

  
                                                   
                                                                        
                                                               
                                                                                
                                                                           
                                                               
                                                       
   
extern errno_t ifnet_inet_defrouter_llreachinfo(ifnet_t interface,
    struct ifnet_llreach_info *pinfo);

  
                                                    
                                                                        
                                                               
                                                                                
                                                                           
                                                               
                                                       
   
extern errno_t ifnet_inet6_defrouter_llreachinfo(ifnet_t interface,
    struct ifnet_llreach_info *pinfo);
#endif                     

   
                                                   
                                                                         
                                                                                     
                                                                                       
                                                           
                                                                             
                                                                        
                                                                                    
  
                                                                             
                                                                              
                     
                                                               
                                                                                         
                                              
                                                                      
                                              
                                                        
   
extern errno_t ifnet_set_capabilities_supported(ifnet_t interface, u_int32_t new_caps,
    u_int32_t mask)
__NKE_API_DEPRECATED;

   
                                               
                                                                                    
                                                                      
                                                                  
   
extern u_int32_t ifnet_capabilities_supported(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                                 
                                                                                        
                                                                               
                                                                                       
                                                                        
                                                                                 
  
                                                                             
                                                                              
                     
  
                                                                                      
                                                                                      
                                                                                          
                                                
  
                                                                                        
                                                                                 
  
                                                                            
                                            
                                                               
                                                                                         
                                              
                                                                      
                                              
                                                        
   
extern errno_t ifnet_set_capabilities_enabled(ifnet_t interface, u_int32_t new_caps,
    u_int32_t mask)
__NKE_API_DEPRECATED;

   
                                             
                                                                                  
                                                                      
                                                                  
   
extern u_int32_t ifnet_capabilities_enabled(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                    
                                                                         
                                                                                 
                                                                           
                                                           
  
                                                                                  
                                                                                 
                                                                           
                                                    
                                        
                                                                             
                                     
                                                        
   
extern errno_t ifnet_set_offload(ifnet_t interface, ifnet_offload_t offload)
__NKE_API_DEPRECATED;

   
                                
                                                                     
                                            
                                                                 
                                                      
   
extern ifnet_offload_t ifnet_offload(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                    
                                                                           
                              
                                        
                                                                           
                                      
                                                                      
                                                        
   
extern errno_t ifnet_set_tso_mtu(ifnet_t interface, sa_family_t family,
    u_int32_t mtuLen)
__NKE_API_DEPRECATED;

   
                                    
                                                                              
                              
                                        
                                                                           
                                      
                                                                           
                                     
                                                        
   
extern errno_t ifnet_get_tso_mtu(ifnet_t interface, sa_family_t family,
    u_int32_t *mtuLen)
__NKE_API_DEPRECATED;

   
                                        
                                                                
                                                                   
   
enum {
	IFNET_WAKE_ON_MAGIC_PACKET = 0x01
};

   
                                       
                                                                                   
                                             
                                        
                                                      
                                                            
                                                        
   
extern errno_t ifnet_set_wake_flags(ifnet_t interface, u_int32_t properties, u_int32_t mask)
__NKE_API_DEPRECATED;

   
                                       
                                                                      
                                        
                                    
   
extern u_int32_t ifnet_get_wake_flags(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                          
                                                                       
                                                                          
                                                                               
                                                                                 
                                                                            
  
                                                                             
                                                                              
                     
                                                              
                                              
                                                 
                                                        
   
extern errno_t ifnet_set_link_mib_data(ifnet_t interface, void *mibData,
    u_int32_t mibLen)
__NKE_API_DEPRECATED;

   
                                          
                                                                         
                                                                               
                              
                                        
                                                                          
                    
                                                                          
                                                                              
                                      
                                                                             
                         
   
extern errno_t ifnet_get_link_mib_data(ifnet_t interface, void *mibData,
    u_int32_t *mibLen)
__NKE_API_DEPRECATED;

   
                                                 
                                                       
                                        
                                                                            
                           
   
extern u_int32_t ifnet_get_link_mib_data_length(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                        
                                                         
                                        
                                                                  
                                           
                                                                     
                                                        
   
extern errno_t ifnet_attach_protocol(ifnet_t interface,
    protocol_family_t protocol_family,
    const struct ifnet_attach_proto_param *proto_details)
__NKE_API_DEPRECATED;

   
                                           
                                                                        
                                                                             
                                                         
                                        
                                                                  
                                           
                                                                     
                                                        
   
extern errno_t ifnet_attach_protocol_v2(ifnet_t interface,
    protocol_family_t protocol_family,
    const struct ifnet_attach_proto_param_v2 *proto_details)
__NKE_API_DEPRECATED;

   
                                        
                                                           
                                        
                                                                      
                        
                                                        
   
extern errno_t ifnet_detach_protocol(ifnet_t interface,
    protocol_family_t protocol_family)
__NKE_API_DEPRECATED;

   
                               
                                                                           
                                                                                 
                                                                           
                                                                           
                                                                               
                                                                                
                                                                          
                                                                         
                                        
                                                                          
                                       
                                                    
                                                                           
                                                                                
                     
                                                                          
                                                                           
                                                                         
                                                        
   
extern errno_t ifnet_output(ifnet_t interface,
    protocol_family_t protocol_family, mbuf_t packet, void *route,
    const struct sockaddr *dest)
__NKE_API_DEPRECATED;

   
                                   
                                                                       
                                                                                 
                                                                              
                                                                            
                                                                               
                                                                               
                                                                             
                                                                             
                                                                    
                                        
                                                                          
                                       
                                                                 
                                                        
   
extern errno_t ifnet_output_raw(ifnet_t interface,
    protocol_family_t protocol_family, mbuf_t packet)
__NKE_API_DEPRECATED;

   
                              
                                                                             
                                                                               
                                                                               
                                                                                
                                                                              
                                        
                                                                    
                                                                            
                                                                           
                                                   
                                                        
   
extern errno_t ifnet_input(ifnet_t interface, mbuf_t first_packet,
    const struct ifnet_stat_increment_param *stats)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                       
                                                                             
                                                                               
                                                                               
                                                                                
                                                                              
                                        
                                                                    
                                                                               
                                                                         
                                                                            
                                                                           
                                                                           
                                       
                                                        
   
extern errno_t ifnet_input_extended(ifnet_t interface, mbuf_t first_packet,
    mbuf_t last_packet, const struct ifnet_stat_increment_param *stats);
#endif                     

   
                              
                                                                             
                        
  
                                                                              
                                                                              
                                               
                                        
                                                                        
                                                                              
                                                         
                                                
                                                      
                                                        
   
extern errno_t ifnet_ioctl(ifnet_t interface, protocol_family_t protocol,
    unsigned long ioctl_code, void *ioctl_arg)
__NKE_API_DEPRECATED;

   
                              
                                                          
                                        
                                                                           
                       
                                                        
   
extern errno_t ifnet_event(ifnet_t interface, struct kern_event_msg *event_ptr)
__NKE_API_DEPRECATED;

   
                                
                                                                          
                                                                              
                                                              
  
                                                                             
                                                                              
                     
                                        
                                
                                                        
   
extern errno_t ifnet_set_mtu(ifnet_t interface, u_int32_t mtu)
__NKE_API_DEPRECATED;

   
                            
                                        
                         
   
extern u_int32_t ifnet_mtu(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                             
                                        
                                              
   
extern u_int8_t ifnet_type(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                    
                    
                                                                             
                                                                              
                     
                                        
                                               
                                                        
   
extern errno_t ifnet_set_addrlen(ifnet_t interface, u_int8_t addrlen)
__NKE_API_DEPRECATED;

   
                                
                                        
                                    
   
extern u_int8_t ifnet_addrlen(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                   
                    
                                                                             
                                                                              
                     
                                        
                                             
                                                        
   
extern errno_t ifnet_set_hdrlen(ifnet_t interface, u_int8_t hdrlen)
__NKE_API_DEPRECATED;

   
                               
                                        
                                   
   
extern u_int8_t ifnet_hdrlen(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                   
                    
                                                                             
                                                                              
                     
                                        
                                      
                                                        
   
extern errno_t ifnet_set_metric(ifnet_t interface, u_int32_t metric)
__NKE_API_DEPRECATED;

   
                               
                                        
                            
   
extern u_int32_t ifnet_metric(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                     
                    
                                                                             
                                                                              
                     
                                        
                                          
                                                        
   
extern errno_t ifnet_set_baudrate(ifnet_t interface, u_int64_t baudrate)
__NKE_API_DEPRECATED;

   
                                 
                                        
                              
   
extern u_int64_t ifnet_baudrate(ifnet_t interface)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
typedef struct if_bandwidths if_bandwidths_t;

  
                                       
                                                                         
                                                                            
                                                                              
                                                      
                                        
                                                                           
                                                                            
                                                  
                                                                         
                                                                            
                                                 
                                                        
   
extern errno_t ifnet_set_bandwidths(ifnet_t interface,
    if_bandwidths_t *output_bw, if_bandwidths_t *input_bw);

  
                                   
                                        
                                                                           
                                                                               
                                        
                                                                         
                                                                               
                                       
                                                        
   
extern errno_t ifnet_bandwidths(ifnet_t interface, if_bandwidths_t *output_bw,
    if_bandwidths_t *input_bw);

typedef struct if_latencies if_latencies_t;

  
                                      
                                                                         
                                                                          
                                                                              
                                                      
                                        
                                                                    
                                                                            
                                                
                                                                  
                                                                            
                                               
                                                        
   
extern errno_t ifnet_set_latencies(ifnet_t interface,
    if_latencies_t *output_lt, if_latencies_t *input_lt);

  
                                  
                                        
                                                                    
                                                                               
                                      
                                                                  
                                                                               
                                     
                                                        
   
extern errno_t ifnet_latencies(ifnet_t interface, if_latencies_t *output_lt,
    if_latencies_t *input_lt);
#endif                     

   
                                       
                    
                                                                             
                                                                              
                     
                                        
                                                                        
                                                                           
                                                                            
                                                        
   
extern errno_t ifnet_stat_increment(ifnet_t interface,
    const struct ifnet_stat_increment_param *counts)
__NKE_API_DEPRECATED;

   
                                          
                    
                                                                           
                                                                                 
                                                                               
                             
  
                                                                               
                                                                              
                       
                                        
                                                                     
                                                                 
                                                                  
                                                        
   
extern errno_t ifnet_stat_increment_in(ifnet_t interface,
    u_int32_t packets_in, u_int32_t bytes_in, u_int32_t errors_in)
__NKE_API_DEPRECATED;

   
                                           
                    
                                                                           
                                                                          
                        
  
                                                                               
                                                                              
                       
                                        
                                                                  
                                                              
                                                                
                                                        
   
extern errno_t ifnet_stat_increment_out(ifnet_t interface,
    u_int32_t packets_out, u_int32_t bytes_out, u_int32_t errors_out)
__NKE_API_DEPRECATED;

   
                                 
                    
                                                                             
                                                                              
                     
  
                                                                               
                                     
                                        
                                           
                                                        
   
extern errno_t ifnet_set_stat(ifnet_t interface,
    const struct ifnet_stats_param *stats)
__NKE_API_DEPRECATED;

   
                             
                                        
                                                 
                                                        
   
extern errno_t ifnet_stat(ifnet_t interface,
    struct ifnet_stats_param *out_stats)
__NKE_API_DEPRECATED;

   
                                        
                                                                             
                                                                         
                                                                            
                                                                          
                                                                         
                                                                      
                                                                       
                                                                             
                                                                              
                                                                                
                                                         
                                                        
   
extern errno_t ifnet_set_promiscuous(ifnet_t interface, int on)
__NKE_API_DEPRECATED;

   
                                         
                                                         
                                        
                                                        
   
extern errno_t ifnet_touch_lastchange(ifnet_t interface)
__NKE_API_DEPRECATED;

   
                                   
                                        
                                                                             
                    
   
extern errno_t ifnet_lastchange(ifnet_t interface, struct timeval *last_change)
__NKE_API_DEPRECATED;

   
                                         
                                                                           
                                                                           
                                                                              
                                                                                
                                                                         
                                                                              
                                                        
                                        
                                                                            
                                                        
   
extern errno_t ifnet_get_address_list(ifnet_t interface, ifaddr_t **addresses)
__NKE_API_DEPRECATED;

   
                                                
                                                                           
                                                                           
                                                                              
                                                                                
                                                                         
                                                                              
                                                               
                                                                              
                                                                                  
                                        
                                                                            
                                                        
   
extern errno_t ifnet_get_address_list_family(ifnet_t interface,
    ifaddr_t **addresses, sa_family_t family)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                               
                                                                         
                                                                             
                                                                        
                                                                          
                                          
                                       
                                                                           
                                                        
   
extern errno_t ifnet_get_inuse_address_list(ifnet_t interface,
    ifaddr_t **addresses);

__private_extern__ errno_t ifnet_get_address_list_family_internal(ifnet_t,
    ifaddr_t **, sa_family_t, int, int, int);
#endif                     

   
                                          
                                                           
                                                                               
                                                         
                                                
   
extern void ifnet_free_address_list(ifaddr_t *addresses)
__NKE_API_DEPRECATED;

   
                                   
                                                                    
                                                                       
                            
                                                                          
                                                           
                                                                           
   
extern errno_t ifnet_set_lladdr(ifnet_t interface, const void *lladdr,
    size_t lladdr_len)
__NKE_API_DEPRECATED;

   
                                          
                                                                        
                                  
                                                                            
                                                                       
                                                                          
                                                  
   
extern errno_t ifnet_lladdr_copy_bytes(ifnet_t interface, void *lladdr,
    size_t length)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                                  
                                                                        
                                                                           
                                                                             
                                                                            
                                                                       
                                                                          
                                                  
   
extern errno_t ifnet_guarded_lladdr_copy_bytes(ifnet_t interface, void *lladdr,
    size_t length);

   
                               
                                                                 
                                                                     
   
extern void *ifnet_lladdr(ifnet_t interface);

#endif                     

   
                                               
                                                                        
                           
                                        
                                                                  
                                                           
                                                                       
   
extern errno_t ifnet_llbroadcast_copy_bytes(ifnet_t interface, void *addr,
    size_t bufferlen, size_t *out_len)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                            
                                                                             
                                                                                 
                                     
                                                                       
                            
                                                                          
                                                           
                                                                       
                                                 
   
extern errno_t ifnet_set_lladdr_and_type(ifnet_t interface, const void *lladdr,
    size_t length, u_char type)
__NKE_API_DEPRECATED;
#endif                     

   
                                          
                                                                             
                                                                                
                                                                            
                                  
                                                                             
                         
                                                                     
                                                                        
                                                                             
                                                                           
                                         
   
extern errno_t ifnet_resolve_multicast(ifnet_t ifp,
    const struct sockaddr *proto_addr, struct sockaddr *ll_addr, size_t ll_len)
__NKE_API_DEPRECATED;

   
                                      
                                                                            
                                                                             
                                                               
                                                                              
                                             
                                        
                                                                               
                                                                            
                                  
                                                                        
                                                        
   
extern errno_t ifnet_add_multicast(ifnet_t interface,
    const struct sockaddr *maddr, ifmultiaddr_t *multicast)
__NKE_API_DEPRECATED;

   
                                         
                                                                           
                                                                                 
                                                                                 
                                                                                 
                                                                                
                        
  
                                                                                 
                                                                                 
                                                                          
                                                               
  
                                                                         
                                                                           
                                                                       
                                    
  
                                                                          
                                                     
                                                      
                                                        
   
extern errno_t ifnet_remove_multicast(ifmultiaddr_t multicast)
__NKE_API_DEPRECATED;

   
                                           
                                                                           
                                                                                
                                                                              
                                                                           
                                                                              
                                    
                                        
                                                                            
                                            
                                                        
   
extern errno_t ifnet_get_multicast_list(ifnet_t interface,
    ifmultiaddr_t **addresses)
__NKE_API_DEPRECATED;

   
                                            
                                                           
                                                                          
                                                       
                                                                             
   
extern void ifnet_free_multicast_list(ifmultiaddr_t *multicasts)
__NKE_API_DEPRECATED;

   
                                     
                                                                             
                                                                                
                       
                                                                           
                              
                                                                           
                                                            
                                                        
   
extern errno_t ifnet_find_by_name(const char *ifname, ifnet_t *interface)
__NKE_API_DEPRECATED;

   
                                 
                                                                           
                                                                              
                                                                             
                                                                             
                                 
                                                                            
                                                                       
                                                                         
                                                                         
                                                  
                                                        
   
extern errno_t ifnet_list_get(ifnet_family_t family, ifnet_t **interfaces,
    u_int32_t *count)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                     
                                                                           
                                                                              
                                                                             
                                                                             
                                                                            
                                                                
                                                                            
                                                                       
                                                                         
                                                                         
                                                  
                                                        
   
extern errno_t ifnet_list_get_all(ifnet_family_t family, ifnet_t **interfaces,
    u_int32_t *count);

#endif                     

   
                                  
                                                                          
                                                                               
                                                                                 
                                                                     
                                 
                                                                                
   
extern void ifnet_list_free(ifnet_t *interfaces)
__NKE_API_DEPRECATED;

                                                                                
                                                                                
                                                                                

   
                                   
                                                                           
                           
                                             
                               
   
extern errno_t ifaddr_reference(ifaddr_t ifaddr)
__NKE_API_DEPRECATED;

   
                                 
                                                                            
                                              
                                             
                               
   
extern errno_t ifaddr_release(ifaddr_t ifaddr)
__NKE_API_DEPRECATED;

   
                                 
                                                          
                                             
                                                              
                                                                  
                               
   
extern errno_t ifaddr_address(ifaddr_t ifaddr, struct sockaddr *out_addr,
    u_int32_t addr_size)
__NKE_API_DEPRECATED;

   
                                 
                                                               
                                             
                                                         
   
extern sa_family_t ifaddr_address_family(ifaddr_t ifaddr)
__NKE_API_DEPRECATED;

   
                                    
                                                                      
                                             
                                                                             
                                                                                 
                               
   
extern errno_t ifaddr_dstaddress(ifaddr_t ifaddr, struct sockaddr *out_dstaddr,
    u_int32_t dstaddr_size)
__NKE_API_DEPRECATED;

   
                                 
                                                          
                                             
                                                                 
                                                                     
                               
   
extern errno_t ifaddr_netmask(ifaddr_t ifaddr, struct sockaddr *out_netmask,
    u_int32_t netmask_size)
__NKE_API_DEPRECATED;

   
                               
                                                                          
                                                                             
                                                                                 
                                                                       
                                 
                                             
                                                                         
   
extern ifnet_t ifaddr_ifnet(ifaddr_t ifaddr)
__NKE_API_DEPRECATED;

   
                                  
                                                                             
                                                                                 
                                                                              
                                                  
                                                      
   
extern ifaddr_t ifaddr_withaddr(const struct sockaddr *address)
__NKE_API_DEPRECATED;

   
                                     
                                                                           
                                                                          
                                                                                 
                                                                              
                                                          
                                                      
   
extern ifaddr_t ifaddr_withdstaddr(const struct sockaddr *destination)
__NKE_API_DEPRECATED;
   
                                 
                                                                            
                                                                                
                                                                                 
                                        
                                              
                                                      
   
extern ifaddr_t ifaddr_withnet(const struct sockaddr *net)
__NKE_API_DEPRECATED;

   
                                   
                                                                         
                                                                             
                                                                           
                                
                                                                      
                                                          
                                                
                                                      
   
extern ifaddr_t ifaddr_withroute(int flags, const struct sockaddr *destination,
    const struct sockaddr *gateway)
__NKE_API_DEPRECATED;

   
                                         
                                                                        
                                                                          
                                                                                 
                                                                              
                                        
                                              
                                                      
   
extern ifaddr_t ifaddr_findbestforaddr(const struct sockaddr *addr,
    ifnet_t interface)
__NKE_API_DEPRECATED;

                                                                                
                                                                                
                                                                                

   
                                    
                                                                            
                         
                                                        
                                                                                 
   
extern errno_t ifmaddr_reference(ifmultiaddr_t ifmaddr)
__NKE_API_DEPRECATED;

   
                                  
                                                                            
                                                                              
                                                                               
                       
                                                        
                                                                                 
   
extern errno_t ifmaddr_release(ifmultiaddr_t ifmaddr)
__NKE_API_DEPRECATED;

   
                                  
                                                                   
                                                     
                                              
                              
   
extern errno_t ifmaddr_address(ifmultiaddr_t ifmaddr,
    struct sockaddr *out_multicast, u_int32_t addr_size)
__NKE_API_DEPRECATED;

   
                                    
                                                               
                                          
                                                                
                                              
                              
   
extern errno_t ifmaddr_lladdress(ifmultiaddr_t ifmaddr,
    struct sockaddr *out_link_layer_multicast, u_int32_t addr_size)
__NKE_API_DEPRECATED;

   
                                
                                                                             
                                                                        
                                                                           
                                                                              
                                                                         
                                                                
                                                        
                                              
   
extern ifnet_t ifmaddr_ifnet(ifmultiaddr_t ifmaddr)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
                                                                                
                                                                                
                                                                                

  
                                         
                                                                              
                                              
                                                         
                                                                               
                                                                           
   
typedef errno_t (*ifnet_clone_create_func)(if_clone_t ifcloner, u_int32_t unit, void *params);

  
                                          
                                                                                      
                                        
                                                   
                                                                           
   
typedef errno_t (*ifnet_clone_destroy_func)(ifnet_t interface);

  
                                   
                                                                             
                                                                             
                                                               
                                                                 
   
struct ifnet_clone_params {
	const char                      *ifc_name;
	ifnet_clone_create_func         ifc_create;
	ifnet_clone_destroy_func        ifc_destroy;
};

  
                                     
                                                     
                                                                             
                                                                                           
                                               
                                      
                                                                    
                                                                                                 
   
extern errno_t ifnet_clone_attach(struct ifnet_clone_params *cloner_params, if_clone_t *ifcloner);

  
                                     
                                                                     
                                                                                           
                                      
   
extern errno_t ifnet_clone_detach(if_clone_t ifcloner);

                                                                                
                                                                                
                                                                                

  
                                        
                                                                         
                                                                          
                                                                              
                                                                             
                                                                           
                                                                             
                                                                             
                                   
                                                                        
                                
                                                 
                                      
   
extern errno_t ifnet_get_local_ports(ifnet_t ifp, u_int8_t *bitfield);

#define IFNET_GET_LOCAL_PORTS_WILDCARDOK        0x01
#define IFNET_GET_LOCAL_PORTS_NOWAKEUPOK        0x02
#define IFNET_GET_LOCAL_PORTS_TCPONLY           0x04
#define IFNET_GET_LOCAL_PORTS_UDPONLY           0x08
#define IFNET_GET_LOCAL_PORTS_RECVANYIFONLY     0x10
#define IFNET_GET_LOCAL_PORTS_EXTBGIDLEONLY     0x20
#define IFNET_GET_LOCAL_PORTS_ACTIVEONLY        0x40
#define IFNET_GET_LOCAL_PORTS_ANYTCPSTATEOK     0x80
  
                                                 
                                                                           
                                                                         
                                                                               
                                                                              
                                                                            
                                                                              
                                                                             
                     
                                                                        
                                
                                                                           
                                                                    
                                                       
                                                                   
                                                                      
                                                                        
                                                              
                                                                       
                                                                         
                                                                       
                                      
                                                                         
                                                                            
                                                                         
                                                                            
                      
                                                                          
                                                                      
                                  
                                                                          
                                                                      
                                        
                                                                       
                                                                             
                                                                         
                                                                          
                                                                         
                              
                                                 
                                      
   
extern errno_t ifnet_get_local_ports_extended(ifnet_t ifp,
    protocol_family_t protocol, u_int32_t flags, u_int8_t *bitfield);

                                                                                
                                      
                                                                                

#define IFNET_MODIDLEN  20
#define IFNET_MODARGLEN 12

  
                                      
                                                                           
                                                                      
                                                      
                                                                            
                                                                             
                                                                       
                                                        
                                                 
                                                                              
                                                                             
                                          
                                                                          
   
extern errno_t ifnet_report_issues(ifnet_t ifp, u_int8_t modid[IFNET_MODIDLEN],
    u_int8_t info[IFNET_MODARGLEN]);

                                                                                
                                                                                
                                                                                
  
                                                                      
                                                                       
                                                         
                    
                                                                      
                                                                 
                                                                               
                                                                        
   
enum {
	IFNET_TX_COMPL_SUCCESS          = 0,                      
	IFNET_TX_COMPL_FAIL             = 1,                                
	IFNET_TX_COMPL_ABORTED          = 2,                                   
	IFNET_TX_COMPL_QFULL            = 3                                  
};

typedef u_int32_t       tx_compl_val_t;

  
                                        
                                                                          
                                                                            
                        
                                                            
                                               
                                                            
   
extern errno_t ifnet_tx_compl_status(ifnet_t ifp, mbuf_t m, tx_compl_val_t val);

  
                                 
                                                                       
                                                            
                                               
   
extern errno_t ifnet_tx_compl(ifnet_t ifp, mbuf_t m);

                                                                                
                                                                                
                                                                                

  
                                             
                                                                         
                                                                             
                      
                                                                               
                            
                                                                           
                          
                                                                           
                                         
                                                                             
                                                                               
                                                                              
                                                                       
                                                                          
   
extern errno_t
ifnet_notice_node_presence(ifnet_t ifp, struct sockaddr *sa, int32_t rssi,
    int lqm, int npm, u_int8_t srvinfo[48]);

  
                                            
                                                                         
                                                                                 
                                                                               
                                                  
                                                                                            
                                                                                        
                                 
                                                                          
   
extern errno_t ifnet_notice_node_absence(ifnet_t ifp, struct sockaddr *sa);

  
                                                
                                                                         
                                                                             
                      
                                                                               
                            
                                                                            
                          
                                                                            
                          
                                                                           
                                         
                                                                             
                                                                               
                                                                              
                                                                       
                                                                          
   
extern errno_t
ifnet_notice_node_presence_v2(ifnet_t ifp, struct sockaddr *sa, struct sockaddr_dl *sdl, int32_t rssi,
    int lqm, int npm, u_int8_t srvinfo[48]);

  
                                              
                                                                                
                                                                                
                                                
                                                                               
                              
                                                                          
   
extern errno_t ifnet_notice_master_elected(ifnet_t ifp);

                                                                                
                                         
                                                                                

  
                                     
                                                                               
                                                                              
                                                                              
                                                                                 
                                                         
                                             
                                                                           
                                                                            
                                                                        
                                                                         
                                                                           
                                                                              
                             
                                                                         
                                                                            
   
extern errno_t
ifnet_set_delegate(ifnet_t ifp, ifnet_t delegated_ifp);

  
                                     
                                                                              
                                             
                                                                        
                                                                      
                                                                       
                                                                          
                                                                             
                                        
                                                                         
                                                                            
   
extern errno_t
ifnet_get_delegate(ifnet_t ifp, ifnet_t *pdelegated_ifp);

                                                                           
                                                                           
                                                                           

  
                                              
                                                                           
                                                     
                                                                          
                                     
                                                                         
                                                                
                                                                     
                                                                         
                                                                             
                                               
                                                                              
                                                                       
                               
                                                                               
                                                                         
                                                     
                                                 
                                                 
                                                                
                                                                      
                                                                    
                                                                          
                                                                                                 
                                                                                       
                                                                                    
                                      
                                                                                   
                                                                                          
                                                                                            
                                                
                                                  
                                                                                        
                                                                                                    
   

#define IFNET_KEEPALIVE_OFFLOAD_FRAME_DATA_SIZE 128
#define IFNET_KEEPALIVE_OFFLOAD_MAX_ADDR_SIZE 16

struct ifnet_keepalive_offload_frame {
	u_int8_t data[IFNET_KEEPALIVE_OFFLOAD_FRAME_DATA_SIZE];                 
#define IFNET_KEEPALIVE_OFFLOAD_FRAME_IPSEC 0x0
#define IFNET_KEEPALIVE_OFFLOAD_FRAME_AIRPLAY 0x1
#define IFNET_KEEPALIVE_OFFLOAD_FRAME_TCP 0x2
	u_int8_t type;                           
	u_int8_t length;                                                  
	u_int16_t interval;                                     
#define IFNET_KEEPALIVE_OFFLOAD_FRAME_ETHERTYPE_IPV4    0x0
#define IFNET_KEEPALIVE_OFFLOAD_FRAME_ETHERTYPE_IPV6    0x1
	u_int8_t ether_type;                              
	u_int8_t keep_cnt;                                         
	u_int16_t keep_retry;                                                                  
	u_int8_t reply_length;                                                        
	u_int8_t addr_length;                                                          
#define  IFNET_KEEPALIVE_OFFLOAD_FLAG_NOWAKEFROMSLEEP   0x01
	u_int8_t flags;
	u_int8_t reserved[1];
	u_int8_t reply_data[IFNET_KEEPALIVE_OFFLOAD_FRAME_DATA_SIZE];                      
	u_int8_t local_addr[IFNET_KEEPALIVE_OFFLOAD_MAX_ADDR_SIZE];                             
	u_int8_t remote_addr[IFNET_KEEPALIVE_OFFLOAD_MAX_ADDR_SIZE];                             
	u_int16_t local_port;                         
	u_int16_t remote_port;                         
	u_int32_t local_seq;                         
	u_int32_t remote_seq;                         
};

  
                                                     
                                                                      
                                                                        
                                                                        
                                                                             
                                                                            
                                                                                 
                                                                            
                                 
                                                                        
                                                                      
                         
                                                                      
                                                                  
                                                           
                                                                       
                                              
                                                                          
                                                                            
                                                                         
                                                   
                                                              
   
extern errno_t ifnet_get_keepalive_offload_frames(ifnet_t ifp,
    struct ifnet_keepalive_offload_frame *frames_array,
    u_int32_t frames_array_count, size_t frame_data_offset,
    u_int32_t *used_frames_count);


  
                                                             
                                                                          
                                                                   
                                                                                
                                                                                 
                                                                 
                                                                        
                                                                  
                                                                  
                                
                             
                             
                              
                             
                              
                                                              
   
extern errno_t ifnet_notify_tcp_keepalive_offload_timeout(ifnet_t ifp,
    struct ifnet_keepalive_offload_frame *frame);

                                                                           
                                                                           
                                                                           
  
                                           
                                                                  
                                                                       
                                                                       
                                                                          
                                                       
                                                               
                                                                      
                                                                      
                                     
                                                                            
                                                              
   
extern errno_t ifnet_link_status_report(ifnet_t ifp, const void *buffer,
    size_t buffer_len);

                                                                           
                                                                           
                                                                           
   
                                             
                                        
                                                                             
                                                                   
                                                              
   
extern errno_t ifnet_set_fastlane_capable(ifnet_t interface, boolean_t capable);

   
                                             
                                        
                                                                             
                                                                                
                                                              
   
extern errno_t ifnet_get_fastlane_capable(ifnet_t interface, boolean_t *capable);

   
                                         
                                       
                                                                            
                             
                                                       
   
extern errno_t ifnet_get_unsent_bytes(ifnet_t interface, int64_t *unsent_bytes);

typedef struct {
	int32_t buf_interface;                                
	int32_t buf_sndbuf;                                    
} ifnet_buffer_status_t;

   
                                          
                                       
                                                                      
                                 
                                                                        
                                                                
   
extern errno_t ifnet_get_buffer_status(const ifnet_t interface,
    ifnet_buffer_status_t *buf_status);

   
                                              
                    
                                                                
                                                                 
  
   
extern void ifnet_normalise_unsent_data(void);

                                                                           
                                                                           
                                                                           

   
                                           
                                        
                                                                               
                                                              
   
extern errno_t ifnet_set_low_power_mode(ifnet_t interface, boolean_t on);

   
                                           
                                        
                                                                        
                                      
                                                              
   
extern errno_t ifnet_get_low_power_mode(ifnet_t interface, boolean_t *on);

   
                                    
                                                    
                                   
                                                   
   
extern errno_t ifnet_touch_lastupdown(ifnet_t interface);

   
                                
                                                                    
                                   
                                                                                      
       
   
extern errno_t ifnet_updown_delta(ifnet_t interface, struct timeval *updown_delta);

                                                                           
                                                                           
                                                                           
   
                                                  
                                                                     
                                         
                                                                         
                                                                       
                            
                                                              
   
extern errno_t ifnet_interface_advisory_report(ifnet_t ifp,
    const struct ifnet_interface_advisory *advisory);

#endif                     

__END_DECLS

#undef __NKE_API_DEPRECATED
#endif                        
