/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
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
 *      @(#)bpf.h	8.1 (Berkeley) 6/10/93
 *	@(#)bpf.h	1.34 (LBL)     6/16/96
 *
 * $FreeBSD: src/sys/net/bpf.h,v 1.21.2.3 2001/08/01 00:23:13 fenner Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _NET_BPF_H_
#define _NET_BPF_H_
#include <sys/param.h>
#include <sys/appleapiopts.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/cdefs.h>
#include <stdint.h>

#ifdef PRIVATE
#include <net/if_var.h>
#include <uuid/uuid.h>

struct bpf_setup_args {
	uuid_t  bsa_uuid;
	char    bsa_ifname[IFNAMSIZ];
};
#endif              

#ifdef KERNEL
#include <sys/kernel_types.h>

#if !defined(__i386__) && !defined(__x86_64__)
#define BPF_ALIGN 1
#else                                               
#define BPF_ALIGN 0
#endif                                               

#if !BPF_ALIGN
#define EXTRACT_SHORT(p)        ((u_int16_t)ntohs(*(u_int16_t *)(void *)p))
#define EXTRACT_LONG(p)         (ntohl(*(u_int32_t *)(void *)p))
#else
#define EXTRACT_SHORT(p) \
	((u_int16_t)\
	        ((u_int16_t)*((u_char *)p+0)<<8|\
	         (u_int16_t)*((u_char *)p+1)<<0))
#define EXTRACT_LONG(p) \
	        ((u_int32_t)*((u_char *)p+0)<<24|\
	         (u_int32_t)*((u_char *)p+1)<<16|\
	         (u_int32_t)*((u_char *)p+2)<<8|\
	         (u_int32_t)*((u_char *)p+3)<<0)
#endif

#endif             

                            
#define BPF_RELEASE 199606

typedef int32_t   bpf_int32;
typedef u_int32_t bpf_u_int32;

  
                                                         
                                  
   
#define BPF_ALIGNMENT sizeof(int32_t)
#define BPF_WORDALIGN(x) (((x)+(BPF_ALIGNMENT-1))&~(BPF_ALIGNMENT-1))

#define BPF_MAXINSNS 512
#define BPF_MAXBUFSIZE 0x80000
#define BPF_MINBUFSIZE 32

  
                           
   
struct bpf_program {
	u_int bf_len;
	struct bpf_insn *bf_insns;
};

#ifdef KERNEL_PRIVATE
  
                                             
                                                 
                                          
   
struct bpf_program64 {
	u_int           bf_len;
	user64_addr_t   bf_insns __attribute__((aligned(8)));
};

struct bpf_program32 {
	u_int           bf_len;
	user32_addr_t   bf_insns;
};
#endif                     

  
                                 
   
struct bpf_stat {
	u_int bs_recv;                                          
	u_int bs_drop;                                         
};

  
                                                                       
                                                                    
                                                                
                                                                      
                                                                    
                                                                     
                                                                     
                               
                                                     
   
struct bpf_version {
	u_short bv_major;
	u_short bv_minor;
};
#if defined(__LP64__)
#include <sys/_types/_timeval32.h>

#define BPF_TIMEVAL timeval32
#else
#define BPF_TIMEVAL timeval
#endif               
                                                    
#define BPF_MAJOR_VERSION 1
#define BPF_MINOR_VERSION 1

#define BIOCGBLEN       _IOR('B',102, u_int)
#define BIOCSBLEN       _IOWR('B',102, u_int)
#define BIOCSETF        _IOW('B',103, struct bpf_program)
#ifdef KERNEL_PRIVATE
#define BIOCSETF64      _IOW('B',103, struct bpf_program64)
#define BIOCSETF32      _IOW('B',103, struct bpf_program32)
#endif                     
#define BIOCFLUSH       _IO('B',104)
#define BIOCPROMISC     _IO('B',105)
#define BIOCGDLT        _IOR('B',106, u_int)
#define BIOCGETIF       _IOR('B',107, struct ifreq)
#define BIOCSETIF       _IOW('B',108, struct ifreq)
#define BIOCSRTIMEOUT   _IOW('B',109, struct timeval)
#ifdef KERNEL_PRIVATE
#define BIOCSRTIMEOUT64 _IOW('B',109, struct user64_timeval)
#define BIOCSRTIMEOUT32 _IOW('B',109, struct user32_timeval)
#endif                     
#define BIOCGRTIMEOUT   _IOR('B',110, struct timeval)
#ifdef KERNEL_PRIVATE
#define BIOCGRTIMEOUT64 _IOR('B',110, struct user64_timeval)
#define BIOCGRTIMEOUT32 _IOR('B',110, struct user32_timeval)
#endif                     
#define BIOCGSTATS      _IOR('B',111, struct bpf_stat)
#define BIOCIMMEDIATE   _IOW('B',112, u_int)
#define BIOCVERSION     _IOR('B',113, struct bpf_version)
#define BIOCGRSIG       _IOR('B',114, u_int)
#define BIOCSRSIG       _IOW('B',115, u_int)
#define BIOCGHDRCMPLT   _IOR('B',116, u_int)
#define BIOCSHDRCMPLT   _IOW('B',117, u_int)
#define BIOCGSEESENT    _IOR('B',118, u_int)
#define BIOCSSEESENT    _IOW('B',119, u_int)
#define BIOCSDLT        _IOW('B',120, u_int)
#define BIOCGDLTLIST    _IOWR('B',121, struct bpf_dltlist)
#ifdef PRIVATE
#define BIOCGETTC       _IOR('B', 122, int)
#define BIOCSETTC       _IOW('B', 123, int)
#define BIOCSEXTHDR     _IOW('B', 124, u_int)
#define BIOCGIFATTACHCOUNT      _IOWR('B', 125, struct ifreq)
#endif              
#define BIOCSETFNR      _IOW('B', 126, struct bpf_program)
#ifdef KERNEL_PRIVATE
#define BIOCSETFNR64    _IOW('B',126, struct bpf_program64)
#define BIOCSETFNR32    _IOW('B',126, struct bpf_program32)
#endif                     
#ifdef PRIVATE
#define BIOCGWANTPKTAP  _IOR('B', 127, u_int)
#define BIOCSWANTPKTAP  _IOWR('B', 127, u_int)
#define BIOCSHEADDROP   _IOW('B', 128, int)
#define BIOCGHEADDROP   _IOR('B', 128, int)
#define BIOCSTRUNCATE   _IOW('B', 129, u_int)
#define BIOCGETUUID     _IOR('B', 130, uuid_t)
#define BIOCSETUP       _IOW('B', 131, struct bpf_setup_args)
#define BIOCSPKTHDRV2   _IOW('B', 132, int)
#define BIOCGPKTHDRV2   _IOW('B', 133, int)
#endif              
  
                                      
   
struct bpf_hdr {
	struct BPF_TIMEVAL bh_tstamp;                   
	bpf_u_int32     bh_caplen;                                      
	bpf_u_int32     bh_datalen;                                    
	u_short         bh_hdrlen;                                          
                                                               
};
#ifdef KERNEL
  
                                                                           
                                                                              
                                                                      
   
#define SIZEOF_BPF_HDR  (sizeof(struct bpf_hdr) <= 20 ? 18 : \
    sizeof(struct bpf_hdr))
#endif
#ifdef PRIVATE
  
                                                
                                                                         
   
struct bpf_hdr_ext {
	struct BPF_TIMEVAL bh_tstamp;                   
	bpf_u_int32     bh_caplen;                                      
	bpf_u_int32     bh_datalen;                                    
	u_short         bh_hdrlen;                                
	u_short         bh_flags;
#define BPF_HDR_EXT_FLAGS_DIR_IN        0x0000
#define BPF_HDR_EXT_FLAGS_DIR_OUT       0x0001
	pid_t           bh_pid;                          
	char            bh_comm[MAXCOMLEN + 1];                      
	u_char          _bh_pad2[1];
	u_char          bh_pktflags;
#define BPF_PKTFLAGS_TCP_REXMT  0x0001
#define BPF_PKTFLAGS_START_SEQ  0x0002
#define BPF_PKTFLAGS_LAST_PKT   0x0004
	u_char          bh_proto;                                           
	bpf_u_int32     bh_svc;                            
	bpf_u_int32     bh_flowid;                                          
	bpf_u_int32     bh_unsent_bytes;                                
	bpf_u_int32     bh_unsent_snd;                                    
};

#define BPF_CONTROL_NAME        "com.apple.net.bpf"

struct bpf_mtag {
	char            bt_comm[MAXCOMLEN];
	pid_t           bt_pid;
	bpf_u_int32     bt_svc;
	unsigned char   bt_direction;
#define BPF_MTAG_DIR_IN         0
#define BPF_MTAG_DIR_OUT        1
};

#endif              

  
                              
   
#define DLT_NULL        0                                        
#define DLT_EN10MB      1                            
#define DLT_EN3MB       2                                        
#define DLT_AX25        3                                
#define DLT_PRONET      4                                      
#define DLT_CHAOS       5                  
#define DLT_IEEE802     6                              
#define DLT_ARCNET      7                   
#define DLT_SLIP        8                           
#define DLT_PPP         9                                    
#define DLT_FDDI        10                
#define DLT_ATM_RFC1483 11                                     
#define DLT_RAW         12                  

  
                                          
                                                                    
                                                               
                                                                  
                 
  
                                                             
                                                          
                                                                 
                                                              
                        
   
#define DLT_SLIP_BSDOS  15                                 
#define DLT_PPP_BSDOS   16                                          

  
                                                         
  
                                                                
                                                                   
                                                           
                                                             
                                                      
  
                                                                  
  
                                  
   

  
                                                                  
                                                                
                                                               
                                                                      
  
                                                                    
                                                               
                                                          
                                                                
                                                              
                                                             
                   
   
#define DLT_PFSYNC      18                                       

#define DLT_ATM_CLIP    19                                       

  
                                                                          
                                                                    
                                                               
   
#define DLT_PPP_SERIAL  50                                                   
#define DLT_PPP_ETHER   51                             

  
                                                                          
                                                                       
                                                                      
                                                                          
             
   
#define DLT_SYMANTEC_FIREWALL   99

  
                                                                 
                                                                      
                                                                     
             
   

  
                                                                  
                                                                  
                                                                    
                                                                 
        
  
                                                                 
                          
   
#define DLT_MATCHING_MIN        104

  
                                                                     
                                                                    
                                                                  
                                                                
                                                              
              
  
                                                                      
                                                  
  
                                                                    
                                                                  
   
#define DLT_C_HDLC      104                     
#define DLT_CHDLC       DLT_C_HDLC

#define DLT_IEEE802_11  105                               

  
                                                                 
                                                                 
                                                                    
   

  
                                                                         
                     
                                                                        
                
   
#define DLT_FRELAY      107

  
                                                                     
                                                                       
  
                                                                  
                                                                   
                                                                   
                                          
   
#define DLT_LOOP        108

  
                                                                 
                                                                 
                                                                    
   

  
                                                                       
                                                                    
                
   
#define DLT_ENC 109

  
                                    
   
#define DLT_LINUX_SLL   113

  
                            
   
#define DLT_LTALK       114

  
                
   
#define DLT_ECONET      115

  
                                          
   
#define DLT_IPFILTER    116

  
                                                                     
                                     
   
#define DLT_PFLOG       117

  
                                     
   
#define DLT_CISCO_IOS   118

  
                                                                        
                                                                 
          
   
#define DLT_PRISM_HEADER        119

  
                                                                       
                                         
   
#define DLT_AIRONET_HEADER      120

  
                                        
   
#define DLT_HHDLC       121

  
                                               
   
#define DLT_IP_OVER_FC  122

  
                                            
   
#define DLT_SUNATM      123

  
                                                                
                   
   
#define DLT_RIO         124                  
#define DLT_PCI_EXP     125                      
#define DLT_AURORA      126                                   

  
                                                                        
                               
   
#ifndef DLT_IEEE802_11_RADIO
#define DLT_IEEE802_11_RADIO    127
#endif

  
                                   
   
#define DLT_TZSP                128                                  

  
                             
   
#define DLT_ARCNET_LINUX        129

  
                                   
   
#define DLT_JUNIPER_MLPPP       130
#define DLT_JUNIPER_MLFR        131
#define DLT_JUNIPER_ES          132
#define DLT_JUNIPER_GGSN        133
#define DLT_JUNIPER_MFR         134
#define DLT_JUNIPER_ATM2        135
#define DLT_JUNIPER_SERVICES    136
#define DLT_JUNIPER_ATM1        137

  
                                                                 
                                                                       
          
  
                               
                           
                                               
                                               
                          
     
  
                                                                  
                                                
   
#define DLT_APPLE_IP_OVER_IEEE1394      138

  
                                                                 
                                                            
   
#define DLT_MTP2_WITH_PHDR      139                                                            
#define DLT_MTP2                140                                      
#define DLT_MTP3                141                                              
#define DLT_SCCP                142                                                      

  
                       
   
#define DLT_DOCSIS      143

  
                           
   
#define DLT_LINUX_IRDA  144

  
                                                             
   
#define DLT_IBM_SP      145
#define DLT_IBM_SN      146

  
                                                                     
                                                                        
                                                                  
                                          
  
                                                                  
                                    
  
                                                                        
                                                                  
                                                                         
                                                                       
                                                                    
                                                                       
                                                                       
                                                   
  
                                                                        
                                                                        
                           
  
                                                                   
                                                           
   
#define DLT_USER0               147
#define DLT_USER1               148
#define DLT_USER2               149
#define DLT_USER3               150
#define DLT_USER4               151
#define DLT_USER5               152
#define DLT_USER6               153
#define DLT_USER7               154
#define DLT_USER8               155
#define DLT_USER9               156
#define DLT_USER10              157
#define DLT_USER11              158
#define DLT_USER12              159
#define DLT_USER13              160
#define DLT_USER14              161
#define DLT_USER15              162

#ifdef PRIVATE
  
                          
   
#define DLT_USER0_APPLE_INTERNAL        DLT_USER0                            
#define DLT_USER1_APPLE_INTERNAL        DLT_USER1                            
#define DLT_PKTAP                       DLT_USER2                            
#define DLT_USER3_APPLE_INTERNAL        DLT_USER3                            
#define DLT_USER4_APPLE_INTERNAL        DLT_USER4                            
#endif              

  
                                                                 
                                                              
                               
  
                                                         
  
                                                             
          
   
#define DLT_IEEE802_11_RADIO_AVS 163                                      

  
                                                      
                                                           
                                                          
                      
   
#define DLT_JUNIPER_MONITOR     164

  
                             
   
#define DLT_BACNET_MS_TP        165

  
                                                                        
  
                                                                           
                                                                 
                                                                    
                                                                      
                                                                    
                                                                       
                                            
  
                                                                      
                                         
   
#define DLT_PPP_PPPD            166

  
                                                                    
                                                  
   
#define DLT_PPP_WITH_DIRECTION  DLT_PPP_PPPD
#define DLT_LINUX_PPP_WITHDIRECTION     DLT_PPP_PPPD

  
                                                      
                                                           
                                                          
                               
   
#define DLT_JUNIPER_PPPOE       167
#define DLT_JUNIPER_PPPOE_ATM   168

#define DLT_GPRS_LLC            169                   
#define DLT_GPF_T               170                                      
#define DLT_GPF_F               171                                      

  
                                                                       
                        
   
#define DLT_GCOM_T1E1           172
#define DLT_GCOM_SERIAL         173

  
                                                      
                                                         
                                                               
   
#define DLT_JUNIPER_PIC_PEER    174

  
                                                                     
                                                    
                                                                    
                         
   
#define DLT_ERF_ETH             175                   
#define DLT_ERF_POS             176                            

  
                                                                  
                                                                    
                                                                  
                                         
   
#define DLT_LINUX_LAPD          177

  
                                                      
                                       
                                                    
                                       
                                                        
   
#define DLT_JUNIPER_ETHER       178
#define DLT_JUNIPER_PPP         179
#define DLT_JUNIPER_FRELAY      180
#define DLT_JUNIPER_CHDLC       181

  
                                  
   
#define DLT_MFR                 182

  
                                                      
                                       
                                                     
                           
   
#define DLT_JUNIPER_VP          183

  
                    
                                                                      
                                           
                                                  
                                                                         
   
#define DLT_A429                184

  
                                                   
                                                                      
                                                            
   
#define DLT_A653_ICM            185

  
                                                               
                                      
   
#define DLT_USB                 186

  
                                                              
               
   
#define DLT_BLUETOOTH_HCI_H4    187

  
                                                                
                          
   
#define DLT_IEEE802_16_MAC_CPS  188

  
                                                               
                                      
   
#define DLT_USB_LINUX           189

  
                                                 
                                                                      
                                                           
                                                             
                                        
   
#define DLT_CAN20B              190

  
                                                                 
                                          
   
#define DLT_IEEE802_15_4_LINUX  191

  
                                               
                                                                      
   
#define DLT_PPI                 192

  
                                                                           
                               
   
#define DLT_IEEE802_16_MAC_CPS_RADIO    193

  
                                                      
                                       
                                                     
                                   
   
#define DLT_JUNIPER_ISM         194

  
                                                                   
                                                                            
   
#define DLT_IEEE802_15_4        195

  
                                                           
                                                                         
   
#define DLT_SITA                196

  
                                                                        
                                                                  
                        
   
#define DLT_ERF                 197

  
                                                                     
                                                    
                          
   
#define DLT_RAIF1               198

  
                                                                       
                                                           
                                   
   
#define DLT_IPMB                199

  
                                                      
                                       
                                                                    
   
#define DLT_JUNIPER_ST          200

  
                                                                    
                                                                 
   
#define DLT_BLUETOOTH_HCI_H4_WITH_PHDR  201

  
                                              
  
                                    
  
                                                          
   
#define DLT_AX25_KISS           202

  
                                                                      
                         
                                                    
   
#define DLT_LAPD                203

  
                                                                    
                                                                
                                                                   
                                    
   
#define DLT_PPP_WITH_DIR        204                                                          
#define DLT_C_HDLC_WITH_DIR     205                     
#define DLT_FRELAY_WITH_DIR     206                      
#define DLT_LAPB_WITH_DIR       207               

  
                                                                   
                                     
   

  
                                                                          
                         
   
#define DLT_IPMB_LINUX          209

  
                                                                  
                                             
   
#define DLT_FLEXRAY             210

  
                                                             
                                                             
                                             
   
#define DLT_MOST                211

  
                                                              
                                                              
                           
   
#define DLT_LIN                 212

  
                                                           
                                                          
   
#define DLT_X2E_SERIAL          213

  
                                                             
                                                                  
   
#define DLT_X2E_XORAYA          214

  
                                                                   
                                                                   
                                                                 
                                                               
                        
  
                                                  
   
#define DLT_IEEE802_15_4_NONASK_PHY     215

  
                                                                
                                                                 
                                                                 
                                                 
   
#define DLT_LINUX_EVDEV         216

  
                                                             
  
                                                    
   
#define DLT_GSMTAP_UM           217
#define DLT_GSMTAP_ABIS         218

  
                                                     
                                                                 
              
   
#define DLT_MPLS                219

  
                                                                      
                                                         
   
#define DLT_USB_LINUX_MMAPPED   220

  
                                                   
                                       
   
#define DLT_DECT                221

  
                                                                    
                                        
  
                                                        
                                                              
                                       
  
   
#define DLT_AOS                 222

  
                                                        
                                         
                
  
                                                  
   
#define DLT_WIHART              223

  
                                                            
                                              
   
#define DLT_FC_2                224

  
                                                               
                                               
  
                                                                   
                                                          
                                                              
                                                                          
                                         
  
                                              
   
#define DLT_FC_2_WITH_FRAME_DELIMS      225

  
                                                                               
  
                                                                          
                        
  
                        
                              
                             
                            
                             
                              
                                
                           
                           
     
  
                                                                 
  
                                                                   
                   
  
                                                                      
                                                                    
           
  
                                                                          
                                                                
                                                                  
  
                                                                   
                  
  
                                                                           
  
                                                                
  
                                                                     
  
                                                                     
                                                                      
                                         
  
                                                                           
                        
   
#define DLT_IPNET               226

  
                                                                         
                                                                         
          
  
                                                     
   
#define DLT_CAN_SOCKETCAN       227

  
                                                                         
                                                                          
   
#define DLT_IPV4                228
#define DLT_IPV6                229

  
                                                                   
                                                                  
                                  
   
#define DLT_IEEE802_15_4_NOFCS  230

  
             
  
                                                
  
            
  
                                                                                    
  
                                                                         
                                                                        
  
                                                                        
  
                                                  
   
#define DLT_DBUS                231

  
                                                      
                                       
   
#define DLT_JUNIPER_VS                  232
#define DLT_JUNIPER_SRX_E2E             233
#define DLT_JUNIPER_FIBRECHANNEL        234

  
                                                                   
                                   
  
                                       
  
                         
  
                                                 
   
#define DLT_DVB_CI              235

  
                                                                   
                                                                    
                                           
   
#define DLT_MUX27010            236

  
                                                     
                            
   
#define DLT_STANAG_5066_D_PDU   237

  
                                                      
                                       
   
#define DLT_JUNIPER_ATM_CEMIC   238

  
                         
                                                                  
  
                                                          
   
#define DLT_NFLOG               239

  
                                                                  
                                                              
                                                           
                                     
  
                                                          
   
#define DLT_NETANALYZER         240

  
                                                                  
                                                               
                                                               
                                                               
            
  
                                                          
   
#define DLT_NETANALYZER_TRANSPARENT     241

  
                                                
  
                                                       
   
#define DLT_IPOIB               242

  
                                                       
  
                                                 
   
#define DLT_MPEG_2_TS           243

  
                                                                  
                                      
  
                                                     
   
#define DLT_NG40                244

  
                                                                    
                                                                        
                                                                    
                          
  
                                                
   
#define DLT_NFC_LLCP            245

  
                                                                 
   
#define DLT_USB_DARWIN          266

#define DLT_MATCHING_MAX        266                                                

  
                             
   
                         
#define BPF_CLASS(code) ((code) & 0x07)
#define         BPF_LD          0x00
#define         BPF_LDX         0x01
#define         BPF_ST          0x02
#define         BPF_STX         0x03
#define         BPF_ALU         0x04
#define         BPF_JMP         0x05
#define         BPF_RET         0x06
#define         BPF_MISC        0x07

                   
#define BPF_SIZE(code)  ((code) & 0x18)
#define         BPF_W           0x00
#define         BPF_H           0x08
#define         BPF_B           0x10
#define BPF_MODE(code)  ((code) & 0xe0)
#define         BPF_IMM         0x00
#define         BPF_ABS         0x20
#define         BPF_IND         0x40
#define         BPF_MEM         0x60
#define         BPF_LEN         0x80
#define         BPF_MSH         0xa0

                    
#define BPF_OP(code)    ((code) & 0xf0)
#define         BPF_ADD         0x00
#define         BPF_SUB         0x10
#define         BPF_MUL         0x20
#define         BPF_DIV         0x30
#define         BPF_OR          0x40
#define         BPF_AND         0x50
#define         BPF_LSH         0x60
#define         BPF_RSH         0x70
#define         BPF_NEG         0x80
#define         BPF_JA          0x00
#define         BPF_JEQ         0x10
#define         BPF_JGT         0x20
#define         BPF_JGE         0x30
#define         BPF_JSET        0x40
#define BPF_SRC(code)   ((code) & 0x08)
#define         BPF_K           0x00
#define         BPF_X           0x08

                                      
#define BPF_RVAL(code)  ((code) & 0x18)
#define         BPF_A           0x10

          
#define BPF_MISCOP(code) ((code) & 0xf8)
#define         BPF_TAX         0x00
#define         BPF_TXA         0x80

  
                                  
   
struct bpf_insn {
	u_short         code;
	u_char          jt;
	u_char          jf;
	bpf_u_int32     k;
};

  
                                      
   
#define BPF_STMT(code, k) { (u_short)(code), 0, 0, k }
#define BPF_JUMP(code, k, jt, jf) { (u_short)(code), jt, jf, k }

#pragma pack(4)

  
                                                          
   
struct bpf_dltlist {
	u_int32_t               bfl_len;                                      
	union {
		u_int32_t       *bflu_list;                        
		u_int64_t       bflu_pad;
	} bfl_u;
};
#define bfl_list bfl_u.bflu_list

#pragma pack()

#ifdef KERNEL_PRIVATE
#define BPF_MIN_PKT_SIZE 40
#define PORT_DNS 53
#define PORT_BOOTPS 67
#define PORT_BOOTPC 68
#define PORT_ISAKMP 500
#define PORT_ISAKMP_NATT 4500                

                          
struct ifnet;
struct mbuf;

#define BPF_PACKET_TYPE_MBUF    0

struct bpf_packet {
	int     bpfp_type;
	void *  bpfp_header;                          
	size_t  bpfp_header_length;
	union {
		struct mbuf     *bpfpu_mbuf;
		void *          bpfpu_ptr;
	} bpfp_u;
#define bpfp_mbuf       bpfp_u.bpfpu_mbuf
#define bpfp_ptr        bpfp_u.bpfpu_ptr
	size_t  bpfp_total_length;                                            
};

extern int      bpf_validate(const struct bpf_insn *, int);
extern void     bpfdetach(struct ifnet *);
extern void     bpfilterattach(int);
extern u_int    bpf_filter(const struct bpf_insn *, u_char *, u_int, u_int);
#endif                     

#ifdef KERNEL
#ifndef BPF_TAP_MODE_T
#define BPF_TAP_MODE_T
   
                           
                                                         
                                                 
                                                    
                                                      
                                                                 
   

enum {
	BPF_MODE_DISABLED       = 0,
	BPF_MODE_INPUT          = 1,
	BPF_MODE_OUTPUT         = 2,
	BPF_MODE_INPUT_OUTPUT   = 3
};
   
                              
                                                                                 
   
typedef u_int32_t bpf_tap_mode;
#endif                      

   
                               
                                                                          
                                                                              
                                                                         
                                                                       
                                                                    
                                                                                
                                             
   
typedef errno_t (*bpf_send_func)(ifnet_t interface, u_int32_t data_link_type,
    mbuf_t packet);

   
                              
                                                                     
                                                                                 
                                                                                
                                                                          
                                                                         
                                                                               
                                                                                
                                                                          
                         
                                                     
                                                               
                                                   
   
typedef errno_t (*bpf_tap_func)(ifnet_t interface, u_int32_t data_link_type,
    bpf_tap_mode direction);

   
                            
                                                                             
                                                                               
                                                                          
                          
                                                             
                                                                           
                                        
                                                                            
   
extern void  bpfattach(ifnet_t interface, u_int data_link_type,
    u_int header_length);

   
                             
                                                                             
                                                                             
                                                                            
                                                                            
                                                                                 
                                                                               
                                                                                
                                                                                 
                                                                           
                                                             
                                                                           
                                        
                                                                            
                                                           
                                                         
   
extern errno_t  bpf_attach(ifnet_t interface, u_int32_t data_link_type,
    u_int32_t header_length, bpf_send_func send, bpf_tap_func tap);

   
                             
                                                                      
                                                                           
                                      
                                                                   
                                                     
                                           
                                                                              
                                                                                 
   
extern void bpf_tap_in(ifnet_t interface, u_int32_t dlt, mbuf_t packet,
    void *header, size_t header_len);

   
                              
                                                                       
                                                                           
                                      
                                                                                 
                                                     
                                           
                                                                              
                                                                                 
   
extern void bpf_tap_out(ifnet_t interface, u_int32_t dlt, mbuf_t packet,
    void *header, size_t header_len);

#endif             

  
                                                                  
   
#define BPF_MEMWORDS 16

#endif                  
