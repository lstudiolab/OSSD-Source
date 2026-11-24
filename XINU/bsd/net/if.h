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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)if.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NET_IF_H_
#define _NET_IF_H_

#include <sys/cdefs.h>
#include <net/net_kev.h>

#define IF_NAMESIZE     16

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#include <sys/appleapiopts.h>
#ifdef __APPLE__

#include <net/if_var.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef PRIVATE
#include <net/if_dl.h>
#include <netinet/in.h>
#include <mach/vm_types.h>
#endif              
#endif

struct if_clonereq {
	int     ifcr_total;                                      
	int     ifcr_count;                                                    
	char    *ifcr_buffer;                                        
};

#ifdef KERNEL_PRIVATE
#define IF_MAXUNIT      0x7fff                        

struct if_clonereq64 {
	int     ifcr_total;                                      
	int     ifcr_count;                                                    
	user64_addr_t ifcru_buffer      __attribute__((aligned(8)));
};

struct if_clonereq32 {
	int     ifcr_total;                                      
	int     ifcr_count;                                                    
	user32_addr_t ifcru_buffer;
};
#endif                     

#define IFF_UP          0x1                                  
#define IFF_BROADCAST   0x2                                          
#define IFF_DEBUG       0x4                                    
#define IFF_LOOPBACK    0x8                                    
#define IFF_POINTOPOINT 0x10                                                  
#define IFF_NOTRAILERS  0x20                                                 
#define IFF_RUNNING     0x40                                     
#define IFF_NOARP       0x80                                                
#define IFF_PROMISC     0x100                                    
#define IFF_ALLMULTI    0x200                                              
#define IFF_OACTIVE     0x400                                         
#define IFF_SIMPLEX     0x800                                             
#define IFF_LINK0       0x1000                                          
#define IFF_LINK1       0x2000                                          
#define IFF_LINK2       0x4000                                          
#define IFF_ALTPHYS     IFF_LINK2                                              
#define IFF_MULTICAST   0x8000                                  

#ifdef PRIVATE
                                                                              
#define IFEF_AUTOCONFIGURING    0x00000001                                             
#define IFEF_ENQUEUE_MULTI      0x00000002                                            
#define IFEF_DELAY_START        0x00000004                                
#define IFEF_PROBE_CONNECTIVITY 0x00000008                                                       
#define IFEF_ADV_REPORT         0x00000010                                              
#define IFEF_IPV6_DISABLED      0x00000020                                         
#define IFEF_ACCEPT_RTADV       0x00000040                                            
#define IFEF_TXSTART            0x00000080                              
#define IFEF_RXPOLL             0x00000100                                             
#define IFEF_VLAN               0x00000200                                           
#define IFEF_BOND               0x00000400                                     
#define IFEF_ARPLL              0x00000800                                    
#define IFEF_CLAT46             0x00001000                           

#define IS_INTF_CLAT46(ifp)     ((ifp) != NULL && ((ifp)->if_eflags & IFEF_CLAT46))
#define INTF_ADJUST_MTU_FOR_CLAT46(intf)                \
    (IS_INTF_CLAT46((intf)) ||                          \
     IS_INTF_CLAT46((intf)->if_delegated.ifp))          \

  
                                                                    
                                                           
                                                             
                                                                 
   
#define IFEF_NOAUTOIPV6LL       0x00002000                                         
#define IFEF_EXPENSIVE          0x00004000                                  
#define IFEF_IPV4_ROUTER        0x00008000                                             
#define IFEF_LOCALNET_PRIVATE   0x00020000                                 
#define IFEF_SERVICE_TRIGGERED  IFEF_LOCALNET_PRIVATE
#define IFEF_IPV6_ND6ALT        0x00040000                                    
#define IFEF_RESTRICTED_RECV    0x00080000                                            
#define IFEF_AWDL               0x00100000                                      
#define IFEF_NOACKPRI           0x00200000                                     
#define IFEF_AWDL_RESTRICTED    0x00400000                                
#define IFEF_2KCL               0x00800000                                                    
#define IFEF_ECN_ENABLE         0x01000000                                                        
#define IFEF_ECN_DISABLE        0x02000000                                                               
#define IFEF_SKYWALK_NATIVE     0x04000000                                  
#define IFEF_3CA                0x08000000                          
#define IFEF_SENDLIST           0x10000000                                    
#define IFEF_DIRECTLINK         0x20000000                                   
#define IFEF_QOSMARKING_ENABLED 0x40000000                                  
#define IFEF_UPDOWNCHANGE       0x80000000                                     

#ifdef XNU_KERNEL_PRIVATE
  
              
   
#define IFXF_WAKE_ON_MAGIC_PACKET       0x00000001                           
#define IFXF_TIMESTAMP_ENABLED          0x00000002                            
#define IFXF_NX_NOAUTO                  0x00000004                           
#define IFXF_LEGACY                     0x00000008                              
#define IFXF_LOW_INTERNET_UL            0x00000010                                       
#define IFXF_LOW_INTERNET_DL            0x00000020                                         
#define IFXF_ALLOC_KPI                  0x00000040                                        
#define IFXF_LOW_POWER                  0x00000080                     
#define IFXF_MPK_LOG                    0x00000100                                 
#define IFXF_CONSTRAINED                0x00000200                                   
#define IFXF_LOW_LATENCY                0x00000400                            
  
                                                                          
                                                                         
                                                                        
   
#define IFEF_AWDL_MASK \
	(IFEF_LOCALNET_PRIVATE | IFEF_IPV6_ND6ALT | IFEF_RESTRICTED_RECV | \
	IFEF_AWDL)
#endif                         
#endif              

#ifdef KERNEL_PRIVATE
  
               
  
                                                                        
                                                                        
                                                                              
                                                                           
                                                                        
                                                             
   
#define IFRF_IDLE_NOTIFY        0x1                                         

                                
#define IFF_CANTCHANGE \
	(IFF_BROADCAST|IFF_POINTOPOINT|IFF_RUNNING|IFF_OACTIVE|\
	    IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI)
#endif                     

  
                                              
  
                               
                                                                         
                                                                      
                                               
                            
                                                                          
                                               
                                               
                                                                               
                                                                          
                                                                            
                                                                           
                                                  
                                                                   
   
#define IFCAP_RXCSUM            0x00001                                 
#define IFCAP_TXCSUM            0x00002                                 
#define IFCAP_VLAN_MTU          0x00004                          
#define IFCAP_VLAN_HWTAGGING    0x00008                                
#define IFCAP_JUMBO_MTU         0x00010                              
#define IFCAP_TSO4              0x00020                                      
#define IFCAP_TSO6              0x00040                                       
#define IFCAP_LRO               0x00080                                   
#define IFCAP_AV                0x00100                               
#define IFCAP_TXSTATUS          0x00200                                       
#define IFCAP_SKYWALK           0x00400                                     
#define IFCAP_HW_TIMESTAMP      0x00800                                
#define IFCAP_SW_TIMESTAMP      0x01000                                
#define IFCAP_CSUM_PARTIAL      0x02000                                   
#define IFCAP_CSUM_ZERO_INVERT  0x04000                                  

#define IFCAP_HWCSUM    (IFCAP_RXCSUM | IFCAP_TXCSUM)
#define IFCAP_TSO       (IFCAP_TSO4 | IFCAP_TSO6)

#define IFCAP_VALID (IFCAP_HWCSUM | IFCAP_TSO | IFCAP_LRO | IFCAP_VLAN_MTU | \
	IFCAP_VLAN_HWTAGGING | IFCAP_JUMBO_MTU | IFCAP_AV | IFCAP_TXSTATUS | \
	IFCAP_SKYWALK | IFCAP_SW_TIMESTAMP | IFCAP_HW_TIMESTAMP | \
	IFCAP_CSUM_PARTIAL | IFCAP_CSUM_ZERO_INVERT)

#define IFQ_MAXLEN      128
#define IFNET_SLOWHZ    1                                    
#define IFQ_TARGET_DELAY        (10ULL * 1000 * 1000)              
#define IFQ_UPDATE_INTERVAL     (100ULL * 1000 * 1000)              

  
                                                                   
                                     
   
struct if_msghdr {
	unsigned short  ifm_msglen;                                          
	unsigned char   ifm_version;                                     
	unsigned char   ifm_type;                         
	int             ifm_addrs;                          
	int             ifm_flags;                             
	unsigned short  ifm_index;                                    
	struct  if_data ifm_data;                                               
};

  
                                                                            
                                     
   
struct ifa_msghdr {
	unsigned short  ifam_msglen;                                         
	unsigned char   ifam_version;                                    
	unsigned char   ifam_type;                        
	int             ifam_addrs;                         
	int             ifam_flags;                             
	unsigned short  ifam_index;                                   
	int             ifam_metric;                             
};

  
                                                                            
                          
   
struct ifma_msghdr {
	unsigned short  ifmam_msglen;                                        
	unsigned char   ifmam_version;                                   
	unsigned char   ifmam_type;                       
	int             ifmam_addrs;                        
	int             ifmam_flags;                            
	unsigned short  ifmam_index;                                  
};

  
                                                                   
              
   
struct if_msghdr2 {
	u_short ifm_msglen;                                               
	u_char  ifm_version;                                     
	u_char  ifm_type;                         
	int     ifm_addrs;                          
	int     ifm_flags;                             
	u_short ifm_index;                                    
	int     ifm_snd_len;                                            
	int     ifm_snd_maxlen;                                   
	int     ifm_snd_drops;                                     
	int     ifm_timer;                                         
	struct if_data64        ifm_data;                                      
};

  
                                                                            
              
   
struct ifma_msghdr2 {
	u_short ifmam_msglen;                                             
	u_char  ifmam_version;                                   
	u_char  ifmam_type;                       
	int     ifmam_addrs;                        
	int     ifmam_flags;                            
	u_short ifmam_index;                                  
	int32_t ifmam_refcount;
};

  
                                 
                                                                          
                                                                   
   
struct ifdevmtu {
	int     ifdm_current;
	int     ifdm_min;
	int     ifdm_max;
};

#pragma pack(4)

  
                              
                                        
  
                                                                               
                                                                                
                                                              
                                                                             
  
                                                                         
                                                                        
                                                                           
                                                                        
                           
  
                        
                                           
                                                              
     
          
                                                            
     
                                                                                
   

struct ifkpi {
	unsigned int    ifk_module_id;
	unsigned int    ifk_type;
	union {
		void            *ifk_ptr;
		int             ifk_value;
#ifdef KERNEL
		u_int64_t       ifk_ptr64;
#endif             
	} ifk_data;
};

                                      
#define IF_WAKE_ON_MAGIC_PACKET         0x01
#ifdef KERNEL_PRIVATE
#define IF_WAKE_VALID_FLAGS IF_WAKE_ON_MAGIC_PACKET
#endif                     


#pragma pack()

  
                                              
                                                      
                                              
                                       
   
struct  ifreq {
#ifndef IFNAMSIZ
#define IFNAMSIZ        IF_NAMESIZE
#endif
	char    ifr_name[IFNAMSIZ];                                      
	union {
		struct  sockaddr ifru_addr;
		struct  sockaddr ifru_dstaddr;
		struct  sockaddr ifru_broadaddr;
		short   ifru_flags;
		int     ifru_metric;
		int     ifru_mtu;
		int     ifru_phys;
		int     ifru_media;
		int     ifru_intval;
		caddr_t ifru_data;
#ifdef KERNEL_PRIVATE
		u_int64_t ifru_data64;                        
#endif                     
		struct  ifdevmtu ifru_devmtu;
		struct  ifkpi   ifru_kpi;
		u_int32_t ifru_wake_flags;
		u_int32_t ifru_route_refcnt;
#ifdef PRIVATE
		int     ifru_link_quality_metric;
#endif              
		int     ifru_cap[2];
#ifdef PRIVATE
		struct {
			uint32_t        ifo_flags;
#define IFRIFOF_BLOCK_OPPORTUNISTIC     0x00000001
			uint32_t        ifo_inuse;
		} ifru_opportunistic;
		u_int64_t ifru_eflags;
		u_int64_t ifru_xflags;
		struct {
			int32_t         ifl_level;
			uint32_t        ifl_flags;
#define IFRLOGF_DLIL                    0x00000001
#define IFRLOGF_FAMILY                  0x00010000
#define IFRLOGF_DRIVER                  0x01000000
#define IFRLOGF_FIRMWARE                0x10000000
			int32_t         ifl_category;
#define IFRLOGCAT_CONNECTIVITY          1
#define IFRLOGCAT_QUALITY               2
#define IFRLOGCAT_PERFORMANCE           3
			int32_t         ifl_subcategory;
		} ifru_log;
		u_int32_t ifru_delegated;
		struct {
			uint32_t        ift_type;
			uint32_t        ift_family;
#define IFRTYPE_FAMILY_ANY              0
#define IFRTYPE_FAMILY_LOOPBACK         1
#define IFRTYPE_FAMILY_ETHERNET         2
#define IFRTYPE_FAMILY_SLIP             3
#define IFRTYPE_FAMILY_TUN              4
#define IFRTYPE_FAMILY_VLAN             5
#define IFRTYPE_FAMILY_PPP              6
#define IFRTYPE_FAMILY_PVC              7
#define IFRTYPE_FAMILY_DISC             8
#define IFRTYPE_FAMILY_MDECAP           9
#define IFRTYPE_FAMILY_GIF              10
#define IFRTYPE_FAMILY_FAITH            11
#define IFRTYPE_FAMILY_STF              12
#define IFRTYPE_FAMILY_FIREWIRE         13
#define IFRTYPE_FAMILY_BOND             14
#define IFRTYPE_FAMILY_CELLULAR         15
#define IFRTYPE_FAMILY_6LOWPAN          16
#define IFRTYPE_FAMILY_UTUN             17
#define IFRTYPE_FAMILY_IPSEC            18
			uint32_t        ift_subfamily;
#define IFRTYPE_SUBFAMILY_ANY           0
#define IFRTYPE_SUBFAMILY_USB           1
#define IFRTYPE_SUBFAMILY_BLUETOOTH     2
#define IFRTYPE_SUBFAMILY_WIFI          3
#define IFRTYPE_SUBFAMILY_THUNDERBOLT   4
#define IFRTYPE_SUBFAMILY_RESERVED      5
#define IFRTYPE_SUBFAMILY_INTCOPROC     6
#define IFRTYPE_SUBFAMILY_QUICKRELAY    7
#define IFRTYPE_SUBFAMILY_DEFAULT       8
		} ifru_type;
#endif              
		u_int32_t ifru_functional_type;
#define IFRTYPE_FUNCTIONAL_UNKNOWN              0
#define IFRTYPE_FUNCTIONAL_LOOPBACK             1
#define IFRTYPE_FUNCTIONAL_WIRED                2
#define IFRTYPE_FUNCTIONAL_WIFI_INFRA           3
#define IFRTYPE_FUNCTIONAL_WIFI_AWDL            4
#define IFRTYPE_FUNCTIONAL_CELLULAR             5
#define IFRTYPE_FUNCTIONAL_INTCOPROC            6
#define IFRTYPE_FUNCTIONAL_COMPANIONLINK        7
#define IFRTYPE_FUNCTIONAL_LAST                 7
#ifdef PRIVATE
		u_int32_t ifru_expensive;
		u_int32_t ifru_constrained;
		u_int32_t ifru_2kcl;
		struct {
			u_int32_t qlen;
			u_int32_t timeout;
		} ifru_start_delay;
		struct if_interface_state       ifru_interface_state;
		u_int32_t ifru_probe_connectivity;
		u_int32_t ifru_ecn_mode;
#define IFRTYPE_ECN_DEFAULT             0
#define IFRTYPE_ECN_ENABLE              1
#define IFRTYPE_ECN_DISABLE             2
		u_int32_t ifru_qosmarking_mode;
#define IFRTYPE_QOSMARKING_MODE_NONE            0
#define IFRTYPE_QOSMARKING_FASTLANE     1                                      
#define IFRTYPE_QOSMARKING_RFC4594      2                                    
		u_int32_t ifru_qosmarking_enabled;
		u_int32_t ifru_disable_output;
		u_int32_t ifru_low_internet;
#define IFRTYPE_LOW_INTERNET_DISABLE_UL_DL      0x0000
#define IFRTYPE_LOW_INTERNET_ENABLE_UL          0x0001
#define IFRTYPE_LOW_INTERNET_ENABLE_DL          0x0002
		int ifru_low_power_mode;
		u_int32_t ifru_tcp_kao_max;
		int ifru_mpk_log;                                    
		u_int32_t ifru_noack_prio;
#endif              
	} ifr_ifru;
#define ifr_addr        ifr_ifru.ifru_addr                   
#define ifr_dstaddr     ifr_ifru.ifru_dstaddr                                 
#define ifr_broadaddr   ifr_ifru.ifru_broadaddr                        
#ifdef __APPLE__
#define ifr_flags       ifr_ifru.ifru_flags                
#else
#define ifr_flags       ifr_ifru.ifru_flags[0]             
#define ifr_prevflags   ifr_ifru.ifru_flags[1]             
#endif                
#define ifr_metric      ifr_ifru.ifru_metric                
#define ifr_mtu         ifr_ifru.ifru_mtu                
#define ifr_phys        ifr_ifru.ifru_phys                         
#define ifr_media       ifr_ifru.ifru_media                         
#define ifr_data        ifr_ifru.ifru_data                                
#define ifr_devmtu      ifr_ifru.ifru_devmtu
#define ifr_intval      ifr_ifru.ifru_intval                       
#ifdef KERNEL_PRIVATE
#define ifr_data64      ifr_ifru.ifru_data64                        
#endif                     
#define ifr_kpi         ifr_ifru.ifru_kpi
#define ifr_wake_flags  ifr_ifru.ifru_wake_flags                        
#define ifr_route_refcnt ifr_ifru.ifru_route_refcnt                             
#ifdef PRIVATE
#define ifr_link_quality_metric ifr_ifru.ifru_link_quality_metric          
#endif              
#define ifr_reqcap      ifr_ifru.ifru_cap[0]                                
#define ifr_curcap      ifr_ifru.ifru_cap[1]                              
#ifdef PRIVATE
#define ifr_opportunistic       ifr_ifru.ifru_opportunistic
#define ifr_eflags      ifr_ifru.ifru_eflags                         
#define ifr_xflags      ifr_ifru.ifru_xflags                      
#define ifr_log         ifr_ifru.ifru_log                                
#define ifr_delegated   ifr_ifru.ifru_delegated                                
#define ifr_expensive   ifr_ifru.ifru_expensive
#define ifr_constrained   ifr_ifru.ifru_constrained
#define ifr_type        ifr_ifru.ifru_type                          
#define ifr_functional_type     ifr_ifru.ifru_functional_type
#define ifr_2kcl        ifr_ifru.ifru_2kcl
#define ifr_start_delay_qlen    ifr_ifru.ifru_start_delay.qlen
#define ifr_start_delay_timeout ifr_ifru.ifru_start_delay.timeout
#define ifr_interface_state     ifr_ifru.ifru_interface_state
#define ifr_probe_connectivity  ifr_ifru.ifru_probe_connectivity
#define ifr_ecn_mode    ifr_ifru.ifru_ecn_mode
#define ifr_qosmarking_mode     ifr_ifru.ifru_qosmarking_mode
#define ifr_fastlane_capable    ifr_qosmarking_mode
#define ifr_qosmarking_enabled  ifr_ifru.ifru_qosmarking_enabled
#define ifr_fastlane_enabled    ifr_qosmarking_enabled
#define ifr_disable_output      ifr_ifru.ifru_disable_output
#define ifr_low_internet        ifr_ifru.ifru_low_internet
#define ifr_low_power_mode      ifr_ifru.ifru_low_power_mode
#define ifr_tcp_kao_max         ifr_ifru.ifru_tcp_kao_max
#define ifr_mpk_log             ifr_ifru.ifru_mpk_log
#define ifr_noack_prio          ifr_ifru.ifru_noack_prio

#endif              
};

#define _SIZEOF_ADDR_IFREQ(ifr) \
	((ifr).ifr_addr.sa_len > sizeof (struct sockaddr) ? \
	(sizeof (struct ifreq) - sizeof (struct sockaddr) + \
	(ifr).ifr_addr.sa_len) : sizeof (struct ifreq))

struct ifaliasreq {
	char    ifra_name[IFNAMSIZ];                                     
	struct  sockaddr ifra_addr;
	struct  sockaddr ifra_broadaddr;
	struct  sockaddr ifra_mask;
};

struct rslvmulti_req {
	struct sockaddr *sa;
	struct sockaddr **llsa;
};

#if !defined(KERNEL) || defined(KERNEL_PRIVATE)
#pragma pack(4)

struct ifmediareq {
	char    ifm_name[IFNAMSIZ];                              
	int     ifm_current;                                       
	int     ifm_mask;                                    
	int     ifm_status;                               
	int     ifm_active;                                 
	int     ifm_count;                                                
	int     *ifm_ulist;                              
};

#pragma pack()
#endif                                

#ifdef KERNEL_PRIVATE
#pragma pack(4)
struct ifmediareq64 {
	char    ifm_name[IFNAMSIZ];                              
	int     ifm_current;                                       
	int     ifm_mask;                                    
	int     ifm_status;                               
	int     ifm_active;                                 
	int     ifm_count;                                                
	user64_addr_t ifmu_ulist __attribute__((aligned(8)));
};

struct ifmediareq32 {
	char    ifm_name[IFNAMSIZ];                              
	int     ifm_current;                                       
	int     ifm_mask;                                    
	int     ifm_status;                               
	int     ifm_active;                                 
	int     ifm_count;                                                
	user32_addr_t ifmu_ulist;                           
};
#pragma pack()
#endif                     


#pragma pack(4)
struct  ifdrv {
	char            ifd_name[IFNAMSIZ];                              
	unsigned long   ifd_cmd;
	size_t          ifd_len;                                               
	void            *ifd_data;
};
#pragma pack()

#ifdef KERNEL_PRIVATE
#pragma pack(4)
struct ifdrv32 {
	char            ifd_name[IFNAMSIZ];                              
	u_int32_t       ifd_cmd;
	u_int32_t       ifd_len;
	user32_addr_t   ifd_data;
};

struct  ifdrv64 {
	char            ifd_name[IFNAMSIZ];                              
	u_int64_t       ifd_cmd;
	u_int64_t       ifd_len;
	user64_addr_t   ifd_data;
};
#pragma pack()
#endif                     

  
                                                              
                                                                   
                                                                   
             
   

#define IFSTATMAX       800                                   
struct ifstat {
	char    ifs_name[IFNAMSIZ];                              
	char    ascii[IFSTATMAX + 1];
};

#if !defined(KERNEL) || defined(KERNEL_PRIVATE)
  
                                         
                                           
                                         
                                      
   
#pragma pack(4)
struct  ifconf {
	int     ifc_len;                                               
	union {
		caddr_t ifcu_buf;
		struct  ifreq *ifcu_req;
	} ifc_ifcu;
};
#pragma pack()
#define ifc_buf ifc_ifcu.ifcu_buf                           
#define ifc_req ifc_ifcu.ifcu_req                                         
#endif                                

#if defined(KERNEL_PRIVATE)
#pragma pack(4)
struct ifconf32 {
	int     ifc_len;                                               
	struct {
		user32_addr_t ifcu_req;
	} ifc_ifcu;
};

struct ifconf64 {
	int     ifc_len;                                               
	struct {
		user64_addr_t ifcu_req  __attribute__((aligned(8)));
	} ifc_ifcu;
};
#pragma pack()
#endif                     

  
                                                
   
struct kev_dl_proto_data {
	struct net_event_data           link_data;
	u_int32_t                       proto_family;
	u_int32_t                       proto_remaining_count;
};

#ifdef PRIVATE
  
                       
  
                                                                    
                                                       
                                                                   
                                                                       
                                                                       
   
enum {
	IFNET_LQM_THRESH_OFF            = (-2),
	IFNET_LQM_THRESH_UNKNOWN        = (-1),
	IFNET_LQM_THRESH_ABORT          = 10,
	IFNET_LQM_THRESH_MINIMALLY_VIABLE = 20,
	IFNET_LQM_THRESH_POOR           = 50,
	IFNET_LQM_THRESH_GOOD           = 100
};
#define IFNET_LQM_THRESH_BAD    IFNET_LQM_THRESH_ABORT

#ifdef XNU_KERNEL_PRIVATE
#define IFNET_LQM_MIN   IFNET_LQM_THRESH_OFF
#define IFNET_LQM_MAX   IFNET_LQM_THRESH_GOOD
#endif                         

  
                                                    
   
struct kev_dl_link_quality_metric_data {
	struct net_event_data   link_data;
	int                     link_quality_metric;
};

#define IF_DESCSIZE     128

  
                               
   
struct if_descreq {
	char                    ifdr_name[IFNAMSIZ];                        
	u_int32_t               ifdr_len;                                      
	u_int8_t                ifdr_desc[IF_DESCSIZE];                  
};

  
                                  
  
                                                                      
                                                                
                                                                 
                                                                 
                     
                                                                 
                                                              
                                                                  
                                                                  
                                  
   
enum {
	IFNET_SCHED_MODEL_NORMAL                = 0,
	IFNET_SCHED_MODEL_DRIVER_MANAGED        = 1,
	IFNET_SCHED_MODEL_FQ_CODEL              = 2,
#ifdef XNU_KERNEL_PRIVATE
	IFNET_SCHED_MODEL_MAX                   = 3,
#endif                         
};

  
                         
   
#define IFLPRF_ALTQ             0x1                                 
#define IFLPRF_DRVMANAGED       0x2                                        

  
                                  
   
struct if_linkparamsreq {
	char            iflpr_name[IFNAMSIZ];                       
	u_int32_t       iflpr_flags;
	u_int32_t       iflpr_output_sched;
	u_int64_t       iflpr_output_tbr_rate;
	u_int32_t       iflpr_output_tbr_percent;
	u_int64_t       iflpr_input_tbr_rate;
	struct if_bandwidths iflpr_output_bw;
	struct if_bandwidths iflpr_input_bw;
	struct if_latencies iflpr_output_lt;
	struct if_latencies iflpr_input_lt;
	struct if_netem_params iflpr_input_netem;
	struct if_netem_params iflpr_output_netem;
};

  
                                  
   
struct if_qstatsreq {
	char            ifqr_name[IFNAMSIZ];                        
	u_int32_t       ifqr_slot;
	void            *ifqr_buf               __attribute__((aligned(8)));
	int              ifqr_len               __attribute__((aligned(8)));
};

  
                         
   
enum {
	IFNET_NPM_THRESH_UNKNOWN        = (-1),
	IFNET_NPM_THRESH_NEAR           = 30,
	IFNET_NPM_THRESH_GENERAL        = 70,
	IFNET_NPM_THRESH_FAR            = 100,
};

  
                                                       
  
                                                
   
enum {
	IFNET_RSSI_UNKNOWN      = ((-2147483647) - 1),                   
};


  
                                                                 
   
struct kev_dl_node_presence {
	struct net_event_data   link_data;
	struct sockaddr_in6     sin6_node_address;
	struct sockaddr_dl      sdl_node_address;
	int32_t                 rssi;
	int                     link_quality_metric;
	int                     node_proximity_metric;
	u_int8_t                node_service_info[48];
};

struct kev_dl_node_absence {
	struct net_event_data   link_data;
	struct sockaddr_in6     sin6_node_address;
	struct sockaddr_dl      sdl_node_address;
};

  
                                   
   
struct if_throttlereq {
	char            ifthr_name[IFNAMSIZ];                       
	u_int32_t       ifthr_level;
};

  
                              
  
                                                                   
                                                              
                                            
                                                                     
                                                             
                                                                
                                                           
                                                              
                                                             
                           
   
enum {
	IFNET_THROTTLE_OFF                      = 0,
	IFNET_THROTTLE_OPPORTUNISTIC            = 1,
#ifdef XNU_KERNEL_PRIVATE
	IFNET_THROTTLE_MAX                      = 2,
#endif                         
};

  
                                   
   
struct if_agentidreq {
	char            ifar_name[IFNAMSIZ];                        
	uuid_t          ifar_uuid;                                               
};

  
                                
   
struct if_agentidsreq {
	char            ifar_name[IFNAMSIZ];                        
	u_int32_t       ifar_count;                                        
	uuid_t          *ifar_uuids;                                      
};

#ifdef BSD_KERNEL_PRIVATE
struct if_agentidsreq32 {
	char            ifar_name[IFNAMSIZ];
	u_int32_t       ifar_count;
	user32_addr_t ifar_uuids;
};
struct if_agentidsreq64 {
	char            ifar_name[IFNAMSIZ];
	u_int32_t       ifar_count;
	user64_addr_t ifar_uuids __attribute__((aligned(8)));
};
#endif                         

  
                             
   
struct if_nexusreq {
	char            ifnr_name[IFNAMSIZ];                        
	uint64_t        ifnr_flags;                                       
	uuid_t          ifnr_netif;                                            
	uuid_t          ifnr_flowswitch;                                   
	uint64_t        ifnr_reserved[5];
};

#define DLIL_MODIDLEN   20                                  
#define DLIL_MODARGLEN  12                                   

  
                                     
   
struct kev_dl_issues {
	struct net_event_data   link_data;
	u_int8_t                modid[DLIL_MODIDLEN];
	u_int64_t               timestamp;
	u_int8_t                info[DLIL_MODARGLEN];
};

  
                                          
   
struct kev_dl_rrc_state {
	struct net_event_data   link_data;
	u_int32_t               rrc_state;
};

  
                                
   
struct kev_dl_low_power_mode {
	struct net_event_data   link_data;
	int                     low_power_event;
};

  
                                                
   
#define IFNET_SIGNATURELEN      20

  
                                        
   
struct if_nsreq {
	char            ifnsr_name[IFNAMSIZ];
	u_int8_t        ifnsr_family;                       
	u_int8_t        ifnsr_len;                       
	u_int16_t       ifnsr_flags;                    
	u_int8_t        ifnsr_data[IFNET_SIGNATURELEN];
};

                                    
struct if_netidreq {
	char            ifnetid_name[IFNAMSIZ];
	u_int8_t        ifnetid_len;
	u_int8_t        ifnetid[IFNET_NETWORK_ID_LEN];
};

#define NAT64_PREFIX_LEN_32     4
#define NAT64_PREFIX_LEN_40     5
#define NAT64_PREFIX_LEN_48     6
#define NAT64_PREFIX_LEN_56     7
#define NAT64_PREFIX_LEN_64     8
#define NAT64_PREFIX_LEN_96     12
#define NAT64_PREFIX_LEN_MAX    NAT64_PREFIX_LEN_96

#define NAT64_MAX_NUM_PREFIXES  4

struct ipv6_prefix {
	struct in6_addr ipv6_prefix;
	uint32_t        prefix_len;
};

struct if_ipv6_address {
	struct in6_addr v6_address;
	uint32_t        v6_prefixlen;
};

                                          
struct if_nat64req {
	char                    ifnat64_name[IFNAMSIZ];
	struct ipv6_prefix      ifnat64_prefixes[NAT64_MAX_NUM_PREFIXES];
};

                                     
struct if_clat46req {
	char                    ifclat46_name[IFNAMSIZ];
	struct if_ipv6_address  ifclat46_addr;
};

  
                                 
  
                                                                       
                                    
  
                                                                       
                                                                        
                                                                              
                                    
   
struct if_order {
	u_int32_t                       ifo_count;
	u_int32_t                       ifo_reserved;
	mach_vm_address_t       ifo_ordered_indices;                         
};

  
                                           
   
struct if_tdmreq {
	char                            iftdm_name[IFNAMSIZ];
	u_int32_t                       iftdm_len;                                    
	struct netsvctype_dscp_map      *iftdm_table;
};

#ifdef BSD_KERNEL_PRIVATE
struct if_tdmreq32 {
	char                    iftdm_name[IFNAMSIZ];
	u_int32_t               iftdm_len;                                    
	user32_addr_t           iftdm_table;
};

struct if_tdmreq64 {
	char                    iftdm_name[IFNAMSIZ];
	u_int32_t               iftdm_len;                                    
	user64_addr_t           iftdm_table __attribute__((aligned(8)));
};
#endif

  
                                  
   
struct if_protolistreq {
	char                    ifpl_name[IFNAMSIZ];
	u_int32_t               ifpl_count;
	u_int32_t               ifpl_reserved;                   
	u_int32_t               *ifpl_list;
};

#ifdef BSD_KERNEL_PRIVATE
struct if_protolistreq32 {
	char                    ifpl_name[IFNAMSIZ];
	u_int32_t               ifpl_count;
	u_int32_t               ifpl_reserved;                   
	user32_addr_t           ifpl_list;
};

struct if_protolistreq64 {
	char                    ifpl_name[IFNAMSIZ];
	u_int32_t               ifpl_count;
	u_int32_t               ifpl_reserved;                   
	user64_addr_t           ifpl_list;
};
#endif                         

#endif              

#ifdef KERNEL
#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_IFADDR);
MALLOC_DECLARE(M_IFMADDR);
#endif
#endif
#endif                                             

#ifndef KERNEL
struct if_nameindex {
	unsigned int     if_index;                     
	char            *if_name;                                             
};

__BEGIN_DECLS
unsigned int     if_nametoindex(const char *);
char            *if_indextoname(unsigned int, char *);
struct           if_nameindex *if_nameindex(void);
void             if_freenameindex(struct if_nameindex *);
__END_DECLS
#endif

#ifdef KERNEL
#include <net/kpi_interface.h>
#endif

#endif                  
