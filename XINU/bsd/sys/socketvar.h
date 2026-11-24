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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1998, 1999 Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)socketvar.h	8.3 (Berkeley) 2/19/95
 * $FreeBSD: src/sys/sys/socketvar.h,v 1.46.2.6 2001/08/31 13:45:49 jlemon Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_SOCKETVAR_H_
#define _SYS_SOCKETVAR_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/types.h>               
#ifdef KERNEL_PRIVATE
#include <sys/queue.h>                                        
#include <sys/select.h>                                         
#include <net/kext_net.h>
#include <sys/ev.h>
#include <uuid/uuid.h>
#ifdef BSD_KERNEL_PRIVATE
#include <sys/eventhandler.h>
#endif                         
#endif                     
#if !KERNEL
#include <TargetConditionals.h>
#endif

typedef u_quad_t so_gen_t;

#ifdef KERNEL_PRIVATE
struct mbuf;
struct socket_filter_entry;
struct protosw;
struct sockif;
struct sockutil;

                                
extern  char netio[], netcon[], netcls[];
#define SOCKET_CACHE_ON
#define SO_CACHE_FLUSH_INTERVAL 1                    
#define SO_CACHE_TIME_LIMIT     (120/SO_CACHE_FLUSH_INTERVAL)              
#define SO_CACHE_MAX_FREE_BATCH 50
#define MAX_CACHED_SOCKETS      512
#define TEMPDEBUG               0
#endif                     

#ifdef PRIVATE
#define SO_TC_STATS_MAX 4

struct data_stats {
	u_int64_t       rxpackets;
	u_int64_t       rxbytes;
	u_int64_t       txpackets;
	u_int64_t       txbytes;
};
#endif              

#ifdef KERNEL_PRIVATE
  
                               
                                           
                                             
                                      
   
struct socket {
	int     so_zone;                                                 
	short   so_type;                                                
	u_short so_error;                                               
	u_int32_t so_options;                                               
	short   so_linger;                                                
	short   so_state;                                                     
	void    *so_pcb;                                            
	struct  protosw *so_proto;                           
	  
                                      
                                                                    
                                                        
                                                                     
                                                                 
                                                           
                                                           
                                                                   
                                                              
    
	struct  socket *so_head;                                           
	TAILQ_HEAD(, socket) so_incomp;                                      
	TAILQ_HEAD(, socket) so_comp;                                       
	TAILQ_ENTRY(socket) so_list;                                        
	short   so_qlen;                                                      
	short   so_incqlen;                                               
                                                   
	short   so_qlimit;                                                 
	short   so_timeo;                                       
	pid_t   so_pgid;                                      
	u_int32_t so_oobmark;                                  
	  
                                   
    
	struct sockbuf {
		u_int32_t       sb_cc;                                      
		u_int32_t       sb_hiwat;                                  
		u_int32_t       sb_mbcnt;                                
		u_int32_t       sb_mbmax;                                      
		u_int32_t       sb_ctl;                                       
		u_int32_t       sb_lowat;                           
		struct mbuf     *sb_mb;                             
		struct mbuf     *sb_mbtail;                                     
		struct mbuf     *sb_lastrecord;                                
		struct socket   *sb_so;                                        
		struct selinfo  sb_sel;                                      
		struct timeval  sb_timeo;                                   
		u_int32_t       sb_flags;                             
		u_int32_t       sb_idealsize;                                 
                                                               
		void    (*sb_upcall)(struct socket *, void *arg, int waitf);
		void    *sb_upcallarg;                             
		u_int32_t       sb_wantlock;                              
		u_int32_t       sb_waiters;                                  
		thread_t        sb_cfil_thread;                            
		u_int32_t       sb_cfil_refs;                          
		u_int32_t       sb_preconn_hiwat;                            
	} so_rcv, so_snd;
#define SB_MAX          (8192*1024)                                           
#define LOW_SB_MAX      (2*9*1024)                                         
                                                             
#define SB_LOCK         0x1                                     
#define SB_NOINTR       0x2                                               
#define SB_RECV         0x4                                 
#define SB_SEL          0x8                                       
#define SB_ASYNC        0x10                                         
#define SB_UPCALL       0x20                                         
#define SB_KNOTE        0x40                                      
#define SB_DROP         0x80                                               
#define SB_UNIX         0x100                                          
#define SB_USRSIZE      0x200                                         
#define SB_AUTOSIZE     0x400                                                 
#define SB_TRIM         0x800                                       
#define SB_NOCOMPRESS   0x1000                                             
#define SB_SNDBYTE_CNT  0x2000                                                     
#define SB_UPCALL_LOCK  0x4000                                                        
#define SB_LIMITED      0x8000                                          
	                                                                     
	caddr_t so_tpcb;                                                     
                                                     

	void            (*so_event)(struct socket *, void *, long);
	void            *so_eventarg;                      
	kauth_cred_t    so_cred;                                           
	                                                                      
	so_gen_t so_gencnt;                                   
	STAILQ_ENTRY(socket) so_cache_ent;                         
	caddr_t         so_saved_pcb;                                        
	u_int32_t       cache_timestamp;                                    

	pid_t           last_pid;                                        
	u_int64_t       last_upid;                                        

	struct socket_filter_entry *so_filt;                  
	u_int32_t       so_flags;                          
#define SOF_NOSIGPIPE           0x00000001
#define SOF_NOADDRAVAIL         0x00000002                                        
#define SOF_PCBCLEARING         0x00000004                              
                                                            
#define SOF_DEFUNCT             0x00000008                                
#define SOF_CLOSEWAIT           0x00000010                                            
#define SOF_REUSESHAREUID       0x00000040                                    
                                                                
#define SOF_MULTIPAGES          0x00000080                                            
#define SOF_ABORTED             0x00000100                                      
#define SOF_OVERFLOW            0x00000200                                     
                                                     
#define SOF_NOTIFYCONFLICT      0x00000400                                    
                                                                
#define SOF_UPCALLCLOSEWAIT     0x00000800                                       
#define SOF_BINDRANDOMPORT      0x00001000                                      
#define SOF_NPX_SETOPTSHUT      0x00002000                                
                                                                          
#define SOF_RECV_TRAFFIC_CLASS  0x00004000                                   
#define SOF_NODEFUNCT           0x00008000                                 
#define SOF_PRIVILEGED_TRAFFIC_CLASS 0x00010000                                  
#define SOF_SUSPENDED           0x00020000                                    
#define SOF_INCOMP_INPROGRESS   0x00040000                                       
#define SOF_NOTSENT_LOWAT       0x00080000                                 
                                                              
#define SOF_KNOTE               0x00100000                                     
#define SOF_FLOW_DIVERT         0x00800000                             
#define SOF_MP_SUBFLOW          0x01000000                                    
#define SOF_MP_SEC_SUBFLOW      0x04000000                            
#define SOF_MP_TRYFAILOVER      0x08000000                      
#define SOF_DELEGATED           0x10000000                                   
#define SOF_CONTENT_FILTER      0x20000000                             

	uint32_t        so_upcallusecount;                                    
	int             so_usecount;                                   
	int             so_retaincnt;
	uint32_t        so_filteruse;                                        
	uint16_t        so_traffic_class;
	int8_t          so_netsvctype;
	uint8_t         so_restrictions;
	thread_t        so_send_filt_thread;

	                        
#define SO_LCKDBG_MAX 4                                                      
	void    *lock_lr[SO_LCKDBG_MAX];                                     
	void    *unlock_lr[SO_LCKDBG_MAX];                                    
	u_int8_t        next_lock_lr;
	u_int8_t        next_unlock_lr;

	u_int16_t       so_pktheadroom;                                     

	u_int32_t       so_ifdenied_notifies;                                   

	struct label    *so_label;                                
	struct label    *so_peerlabel;                                        
	thread_t        so_background_thread;                        
                                                                      
	struct data_stats so_tc_stats[SO_TC_STATS_MAX];
	struct klist    so_klist;                                             

	struct flow_divert_pcb  *so_fd_pcb;                                    

#if CONTENT_FILTER
	struct cfil_info    *so_cfil;
	struct cfil_db      *so_cfil_db;
	u_int32_t           so_state_change_cnt;                                        
#endif

	u_int32_t       so_eventmask;                           

	pid_t           e_pid;                                          
	u_int64_t       e_upid;                                          
#if XNU_TARGET_OS_OSX
	pid_t           so_rpid;                                            
#endif                        

	uuid_t          last_uuid;                                        
	uuid_t          e_uuid;                                      
	uuid_t          so_vuuid;                                           
#if XNU_TARGET_OS_OSX
	uuid_t          so_ruuid;                                            
#endif                        

	int32_t         so_policy_gencnt;                         

	u_int32_t       so_flags1;
#define SOF1_POST_FALLBACK_SYNC         0x00000001                      
#define SOF1_AWDL_PRIVILEGED            0x00000002             
#define SOF1_IF_2KCL                    0x00000004                                      
#define SOF1_DEFUNCTINPROG              0x00000008
#define SOF1_DATA_IDEMPOTENT            0x00000010                              
#define SOF1_PRECONNECT_DATA            0x00000020                                  
#define SOF1_EXTEND_BK_IDLE_WANTED      0x00000040                 
#define SOF1_EXTEND_BK_IDLE_INPROG      0x00000080             
#define SOF1_CACHED_IN_SOCK_LAYER       0x00000100                                    
#define SOF1_TFO_REWIND                 0x00000200                             
#define SOF1_CELLFALLBACK               0x00000400                                 
#define SOF1_QOSMARKING_ALLOWED         0x00000800                             
#define SOF1_TC_NET_SERV_TYPE           0x00001000                                                   
#define SOF1_TRAFFIC_MGT_SO_BACKGROUND  0x00002000                        
#define SOF1_TRAFFIC_MGT_TCP_RECVBG     0x00004000                                            
#define SOF1_QOSMARKING_POLICY_OVERRIDE 0x00008000                                         
#define SOF1_DATA_AUTHENTICATED         0x00010000                                       
#define SOF1_ACCEPT_LIST_HELD           0x00020000                                                          
#define SOF1_CONTENT_FILTER_SKIP        0x00040000                                                          
#define SOF1_HAS_NECP_CLIENT_UUID       0x00080000                                  
#define SOF1_IN_KERNEL_SOCKET           0x00100000                                       
#define SOF1_CONNECT_COUNTED            0x00200000                                 
#define SOF1_DNS_COUNTED                0x00400000                                         
#define SOF1_MPKL_SEND_INFO             0x00800000                                      
#define SOF1_INBOUND                    0x01000000                                     
#define SOF1_WANT_KEV_SOCK_CLOSED       0x02000000                                                 
#define SOF1_FLOW_DIVERT_SKIP           0x04000000                                                        

	u_int64_t       so_extended_bk_start;

	u_int8_t        so_log_seqn;                                                            
	uuid_t          so_mpkl_send_uuid;
	uint8_t         so_mpkl_send_proto;
};

                                       

#define _MIN_NXT_CMSGHDR_PTR(cmsg)                                      \
	((char *)(cmsg) +                                               \
	    __DARWIN_ALIGN32((__uint32_t)(cmsg)->cmsg_len) +            \
	    __DARWIN_ALIGN32(sizeof(struct cmsghdr)))

#define M_FIRST_CMSGHDR(m)                                              \
	((char *)(m) != (char *)0L &&                                   \
	(size_t)(m)->m_len >= sizeof (struct cmsghdr) &&                \
	(socklen_t)(m)->m_len >=                                        \
	__DARWIN_ALIGN32(((struct cmsghdr *)(void *)(m)->m_data)->cmsg_len) ? \
	(struct cmsghdr *)(void *)(m)->m_data :	(struct cmsghdr *)0L)

#define M_NXT_CMSGHDR(m, cmsg)                                          \
	((char *)(cmsg) == (char *)0L ? M_FIRST_CMSGHDR(m) :            \
	_MIN_NXT_CMSGHDR_PTR(cmsg) > ((char *)(m)->m_data) + (m)->m_len ||  \
	_MIN_NXT_CMSGHDR_PTR(cmsg) < (char *)(m)->m_data ?              \
	(struct cmsghdr *)0L            :                               \
	(struct cmsghdr *)(void *)((unsigned char *)(cmsg) +            \
	__DARWIN_ALIGN32((__uint32_t)(cmsg)->cmsg_len)))

  
                     
   
#define SS_NOFDREF              0x0001                                  
#define SS_ISCONNECTED          0x0002                                  
#define SS_ISCONNECTING         0x0004                                        
#define SS_ISDISCONNECTING      0x0008                                   
#define SS_CANTSENDMORE         0x0010                                    
#define SS_CANTRCVMORE          0x0020                                         
#define SS_RCVATMARK            0x0040                        

#define SS_PRIV                 0x0080                                        
#define SS_NBIO                 0x0100                        
#define SS_ASYNC                0x0200                        
#define SS_ISCONFIRMING         0x0400                                         
#define SS_INCOMP               0x0800                                         
#define SS_COMP                 0x1000                                       
#define SS_ISDISCONNECTED       0x2000                                     
#define SS_DRAINING             0x4000                                     
                                                           
#define SS_DEFUNCT              0x8000                                
#endif                     

#if defined(__LP64__)
#define _XSOCKET_PTR(x)         u_int32_t
#else
#define _XSOCKET_PTR(x)         x
#endif

#ifdef PRIVATE
                                                          
#define SOCKEV_CONNECTED        0x00000001                
#define SOCKEV_DISCONNECTED     0x00000002                   
#endif              

#pragma pack(4)

struct xsockbuf {
	u_int32_t       sb_cc;
	u_int32_t       sb_hiwat;
	u_int32_t       sb_mbcnt;
	u_int32_t       sb_mbmax;
	int32_t         sb_lowat;
	short           sb_flags;
	short           sb_timeo;
};

  
                                                                      
   
struct  xsocket {
	u_int32_t               xso_len;                                      
	_XSOCKET_PTR(struct socket *) xso_so;                                  
	short                   so_type;
	short                   so_options;
	short                   so_linger;
	short                   so_state;
	_XSOCKET_PTR(caddr_t)   so_pcb;                                        
	int                     xso_protocol;
	int                     xso_family;
	short                   so_qlen;
	short                   so_incqlen;
	short                   so_qlimit;
	short                   so_timeo;
	u_short                 so_error;
	pid_t                   so_pgid;
	u_int32_t               so_oobmark;
	struct xsockbuf         so_rcv;
	struct xsockbuf         so_snd;
	uid_t                   so_uid;                  
};

#if XNU_TARGET_OS_OSX || KERNEL || !(TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
struct  xsocket64 {
	u_int32_t               xso_len;                                      
	u_int64_t               xso_so;                                        
	short                   so_type;
	short                   so_options;
	short                   so_linger;
	short                   so_state;
	u_int64_t               so_pcb;                                        
	int                     xso_protocol;
	int                     xso_family;
	short                   so_qlen;
	short                   so_incqlen;
	short                   so_qlimit;
	short                   so_timeo;
	u_short                 so_error;
	pid_t                   so_pgid;
	u_int32_t               so_oobmark;
	struct xsockbuf         so_rcv;
	struct xsockbuf         so_snd;
	uid_t                   so_uid;                  
};
#endif                                                                                 

#ifdef PRIVATE
#define XSO_SOCKET      0x001
#define XSO_RCVBUF      0x002
#define XSO_SNDBUF      0x004
#define XSO_STATS       0x008
#define XSO_INPCB       0x010
#define XSO_TCPCB       0x020
#define XSO_KCREG       0x040
#define XSO_KCB         0x080
#define XSO_EVT         0x100

struct  xsocket_n {
	u_int32_t               xso_len;                                      
	u_int32_t               xso_kind;                       
	u_int64_t               xso_so;                                        
	short                   so_type;
	u_int32_t               so_options;
	short                   so_linger;
	short                   so_state;
	u_int64_t               so_pcb;                                        
	int                     xso_protocol;
	int                     xso_family;
	short                   so_qlen;
	short                   so_incqlen;
	short                   so_qlimit;
	short                   so_timeo;
	u_short                 so_error;
	pid_t                   so_pgid;
	u_int32_t               so_oobmark;
	uid_t                   so_uid;                  
	pid_t                   so_last_pid;
	pid_t                   so_e_pid;
};

struct xsockbuf_n {
	u_int32_t               xsb_len;                                      
	u_int32_t               xsb_kind;                                     
	u_int32_t               sb_cc;
	u_int32_t               sb_hiwat;
	u_int32_t               sb_mbcnt;
	u_int32_t               sb_mbmax;
	int32_t                 sb_lowat;
	short                   sb_flags;
	short                   sb_timeo;
};

struct xsockstat_n {
	u_int32_t               xst_len;                                      
	u_int32_t               xst_kind;                      
	struct data_stats       xst_tc_stats[SO_TC_STATS_MAX];
};

  
                           
   
struct soextbkidlestat {
	u_int32_t       so_xbkidle_maxperproc;
	u_int32_t       so_xbkidle_time;
	u_int32_t       so_xbkidle_rcvhiwat;
	int32_t         so_xbkidle_notsupp;
	int32_t         so_xbkidle_toomany;
	int32_t         so_xbkidle_wantok;
	int32_t         so_xbkidle_active;
	int32_t         so_xbkidle_nocell;
	int32_t         so_xbkidle_notime;
	int32_t         so_xbkidle_forced;
	int32_t         so_xbkidle_resumed;
	int32_t         so_xbkidle_expired;
	int32_t         so_xbkidle_resched;
	int32_t         so_xbkidle_nodlgtd;
	int32_t         so_xbkidle_drained;
};
#endif              

#pragma pack()

#ifdef KERNEL_PRIVATE
#include <sys/kpi_mbuf.h>

  
                                                                 
                                                         
   
enum sopt_dir { SOPT_GET, SOPT_SET };
struct sockopt {
	enum    sopt_dir sopt_dir;                              
	int     sopt_level;                                      
	int     sopt_name;                                      
	user_addr_t sopt_val;                                    
	size_t  sopt_valsize;                                            
	struct  proc *sopt_p;                                          
};

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_PCB);
MALLOC_DECLARE(M_SONAME);
#endif                     

#ifdef BSD_KERNEL_PRIVATE
struct cmsghdr;
extern boolean_t is_cmsg_valid(struct mbuf *control, struct cmsghdr *cmsg);

  
                                                   
                                                            
                                                             
                                                           
                                               
   
struct kextcb {
	struct kextcb *e_next;                                       
	void *e_fcb;                                                   
	struct NFDescriptor *e_nfd;                         
	                                           
	struct sockif *e_soif;                                
	struct sockutil *e_sout;                                       
};
#define EXT_NULL        0x0                                    

                                       
#define SO_FILT_HINT_LOCKED             0x00000001                                    
#define SO_FILT_HINT_CONNRESET          0x00000002                             
#define SO_FILT_HINT_CANTRCVMORE        0x00000004                                
#define SO_FILT_HINT_CANTSENDMORE       0x00000008                                 
#define SO_FILT_HINT_TIMEOUT            0x00000010                   
#define SO_FILT_HINT_NOSRCADDR          0x00000020                                    
#define SO_FILT_HINT_IFDENIED           0x00000040                                   
#define SO_FILT_HINT_SUSPEND            0x00000080                                  
#define SO_FILT_HINT_RESUME             0x00000100                                
#define SO_FILT_HINT_KEEPALIVE          0x00000200                                  
#define SO_FILT_HINT_ADAPTIVE_WTIMO     0x00000400                                      
#define SO_FILT_HINT_ADAPTIVE_RTIMO     0x00000800                                     
#define SO_FILT_HINT_CONNECTED          0x00001000                               
#define SO_FILT_HINT_DISCONNECTED       0x00002000                                  
#define SO_FILT_HINT_CONNINFO_UPDATED   0x00004000                                   
#define SO_FILT_HINT_MPFAILOVER         0x00008000                              
#define SO_FILT_HINT_MPSTATUS           0x00010000                            
#define SO_FILT_HINT_MUSTRST            0x00020000                                   
#define SO_FILT_HINT_MPCANTRCVMORE      0x00040000                               
#define SO_FILT_HINT_NOTIFY_ACK         0x00080000                                  
#define SO_FILT_HINT_MP_SUB_ERROR       0x00100000                                    

#define SO_FILT_HINT_BITS \
	"\020\1LOCKED\2CONNRESET\3CANTRCVMORE\4CANTSENDMORE\5TIMEOUT"   \
	"\6NOSRCADDR\7IFDENIED\10SUSPEND\11RESUME\12KEEPALIVE\13AWTIMO" \
	"\14ARTIMO\15CONNECTED\16DISCONNECTED\17CONNINFO_UPDATED"       \
	"\20MPFAILOVER\21MPSTATUS\22MUSTRST\23MPCANTRCVMORE\24NOTIFYACK"\
	"\25MPSUBERROR"

                                                          
#define SO_FILT_HINT_EV                                                 \
	(SO_FILT_HINT_CONNRESET | SO_FILT_HINT_CANTRCVMORE |            \
	SO_FILT_HINT_CANTSENDMORE | SO_FILT_HINT_TIMEOUT |              \
	SO_FILT_HINT_NOSRCADDR | SO_FILT_HINT_IFDENIED |                \
	SO_FILT_HINT_SUSPEND | SO_FILT_HINT_RESUME |                    \
	SO_FILT_HINT_KEEPALIVE | SO_FILT_HINT_ADAPTIVE_WTIMO |          \
	SO_FILT_HINT_ADAPTIVE_RTIMO | SO_FILT_HINT_CONNECTED |          \
	SO_FILT_HINT_DISCONNECTED | SO_FILT_HINT_CONNINFO_UPDATED |     \
	SO_FILT_HINT_NOTIFY_ACK)

#if SENDFILE
struct sf_buf {
	SLIST_ENTRY(sf_buf) free_list;                                 
	int             refcnt;                              
	struct          vm_page *m;                                
	vm_offset_t     kva;                               
};
#endif               

#define SBLASTRECORDCHK(sb, s)                                          \
	if (socket_debug) sblastrecordchk(sb, s);

#define SBLASTMBUFCHK(sb, s)                                            \
	if (socket_debug) sblastmbufchk(sb, s);

#define SB_EMPTY_FIXUP(sb) {                                            \
	if ((sb)->sb_mb == NULL) {                                      \
	        (sb)->sb_mbtail = NULL;                                 \
	        (sb)->sb_lastrecord = NULL;                             \
	}                                                               \
}

#define SB_MB_CHECK(sb) do {                                            \
	if (((sb)->sb_mb != NULL &&                                     \
	    (sb)->sb_cc == 0) ||                                        \
	    ((sb)->sb_mb == NULL && (sb)->sb_cc > 0))                   \
	        panic("corrupt so_rcv: sb_mb %p sb_cc %d\n",            \
	            (sb)->sb_mb, (sb)->sb_cc);                          \
} while (0)

#define SODEFUNCTLOG(fmt, ...)  do {            \
	if (sodefunctlog)                       \
	        printf(fmt, __VA_ARGS__);       \
} while (0)

#define SOTHROTTLELOG(fmt, ...) do {            \
	if (sothrottlelog)                      \
	        printf(fmt, __VA_ARGS__);       \
} while (0)

  
                                        
   
#define SOTCDB_RESERVED         0x01
#define SOTCDB_NO_MTC           0x02                                           
#define SOTCDB_NO_SENDTCPBG     0x04                                                           
#define SOTCDB_NO_LCLTST        0x08                                                            
#define SOTCDB_NO_DSCPTST       0x10                                           
#define SOTCDB_NO_RECVTCPBG     0x20                                                       
#define SOTCDB_NO_PRIVILEGED    0x40                                            

#define SOCK_DOM(so)                    ((so)->so_proto->pr_domain->dom_family)
#define SOCK_TYPE(so)                   ((so)->so_proto->pr_type)
#define SOCK_PROTO(so)                  ((so)->so_proto->pr_protocol)

#define SOCK_CHECK_DOM(so, dom)         (SOCK_DOM(so) == (dom))
#define SOCK_CHECK_TYPE(so, type)       (SOCK_TYPE(so) == (type))
#define SOCK_CHECK_PROTO(so, proto)     (SOCK_PROTO(so) == (proto))

  
                             
   
struct so_procinfo {
	pid_t           spi_pid;
	pid_t           spi_epid;
	uuid_t          spi_uuid;
	uuid_t          spi_euuid;
	int             spi_delegated;
	char            spi_proc_name[MAXCOMLEN + 1];
	char            spi_e_proc_name[MAXCOMLEN + 1];
};

extern u_int32_t sb_max;
extern so_gen_t so_gencnt;
extern int socket_debug;
extern int sosendjcl;
extern int sosendjcl_ignore_capab;
extern int sodefunctlog;
extern int sothrottlelog;
extern int sorestrictrecv;
extern int sorestrictsend;
extern int somaxconn;
extern uint32_t tcp_autosndbuf_max;
extern uint32_t tcp_autosndbuf_inc;
extern u_int32_t sotcdb;
extern u_int32_t net_io_policy_log;
extern u_int32_t net_io_policy_throttle_best_effort;
#if CONFIG_PROC_UUID_POLICY
extern u_int32_t net_io_policy_uuid;
#endif                              

extern struct soextbkidlestat soextbkidlestat;

#endif                         

struct mbuf;
struct sockaddr;
struct ucred;
struct uio;

#define SOCK_MSG_SA 0x01
#define SOCK_MSG_CONTROL 0x02
#define SOCK_MSG_DATA 0x04

struct recv_msg_elem {
	struct uio *uio;
	struct sockaddr *psa;
	struct mbuf *controlp;
	int which;
	int flags;
};

  
                               
   
__BEGIN_DECLS
              
extern int sbappendaddr(struct sockbuf *sb, struct sockaddr *asa,
    struct mbuf *m0, struct mbuf *control, int *error_out);
extern int sbappendchain(struct sockbuf *sb, struct mbuf *m, int space);
extern int sbappendrecord(struct sockbuf *sb, struct mbuf *m0);
extern int sbappendrecord_nodrop(struct sockbuf *sb, struct mbuf *m0);
extern void sbflush(struct sockbuf *sb);
extern int sbspace(struct sockbuf *sb);
extern int soabort(struct socket *so);
extern void socantrcvmore(struct socket *so);
extern void socantsendmore(struct socket *so);
extern int sodisconnect(struct socket *so);
extern void sofree(struct socket *so);
extern void sofreelastref(struct socket *, int);
extern void soisconnected(struct socket *so);
extern boolean_t socanwrite(struct socket *so);
extern void soisconnecting(struct socket *so);
extern void soisdisconnected(struct socket *so);
extern void soisdisconnecting(struct socket *so);
extern struct socket *sonewconn(struct socket *head, int connstatus,
    const struct sockaddr *from);
extern int sopoll(struct socket *so, int events, struct ucred *cred, void *wql);
extern int sooptcopyin(struct sockopt *sopt, void *data, size_t len,
    size_t minlen)
__attribute__ ((warn_unused_result));
extern int sooptcopyout(struct sockopt *sopt, void *data, size_t len)
__attribute__ ((warn_unused_result));
extern int soopt_cred_check(struct socket *so, int priv, boolean_t allow_root,
    boolean_t ignore_delegate);
extern int soreceive(struct socket *so, struct sockaddr **paddr,
    struct uio *uio, struct mbuf **mp0, struct mbuf **controlp, int *flagsp);
extern int soreserve(struct socket *so, uint32_t sndcc, uint32_t rcvcc);
extern void soreserve_preconnect(struct socket *so, unsigned int pre_cc);
extern void sorwakeup(struct socket *so);
extern int sosend(struct socket *so, struct sockaddr *addr, struct uio *uio,
    struct mbuf *top, struct mbuf *control, int flags);
extern int sosend_reinject(struct socket *so, struct sockaddr *addr, struct mbuf *top,
    struct mbuf *control, uint32_t sendflags);
extern int sosend_list(struct socket *so, struct uio **uio, u_int uiocnt,
    int flags);
extern int soreceive_list(struct socket *so, struct recv_msg_elem *msgarray,
    u_int msgcnt, int *flags);
extern void sonullevent(struct socket *so, void *arg, long hint);
extern struct mbuf *sbconcat_mbufs(struct sockbuf *sb, struct sockaddr *asa, struct mbuf *m0,
    struct mbuf *control);


__END_DECLS

#ifdef BSD_KERNEL_PRIVATE
struct file;
struct filedesc;
struct so_tcdbg;

__BEGIN_DECLS
                  
extern void socketinit(void);
extern struct sockaddr *dup_sockaddr(struct sockaddr *sa, int canwait);
extern int getsock(struct filedesc *fdp, int fd, struct file **fpp);
extern int sockargs(struct mbuf **mp, user_addr_t data, socklen_t buflen, int type);
extern void get_sockev_state(struct socket *, u_int32_t *);
extern void so_update_last_owner_locked(struct socket *, struct proc *);
extern void so_update_policy(struct socket *);
extern void so_acquire_accept_list(struct socket *, struct socket *);
extern void so_release_accept_list(struct socket *);

extern int sbappend(struct sockbuf *sb, struct mbuf *m);
extern int sbappend_nodrop(struct sockbuf *sb, struct mbuf *m);
extern int sbappendstream(struct sockbuf *sb, struct mbuf *m);
extern int sbappendcontrol(struct sockbuf *sb, struct mbuf *m0,
    struct mbuf *control, int *error_out);
extern int sbappendstream_rcvdemux(struct socket *so, struct mbuf *m);
#if MPTCP
extern int sbappendmptcpstream_rcv(struct sockbuf *sb, struct mbuf *m);
#endif            
extern void sbcheck(struct sockbuf *sb);
extern void sblastmbufchk(struct sockbuf *, const char *);
extern void sblastrecordchk(struct sockbuf *, const char *);
extern struct mbuf *sbcreatecontrol(caddr_t p, int size, int type, int level);
extern struct mbuf **sbcreatecontrol_mbuf(caddr_t p, int size, int type,
    int level, struct mbuf **m);
extern void sbdrop(struct sockbuf *sb, int len);
extern void sbdroprecord(struct sockbuf *sb);
extern void sbrelease(struct sockbuf *sb);
extern int sbreserve(struct sockbuf *sb, u_int32_t cc);
extern void sbtoxsockbuf(struct sockbuf *sb, struct xsockbuf *xsb);
extern int sbwait(struct sockbuf *sb);
extern void sbwakeup(struct sockbuf *sb);
extern void sb_empty_assert(struct sockbuf *, const char *);
extern int sb_notify(struct sockbuf *sb);
extern void sballoc(struct sockbuf *sb, struct mbuf *m);
extern void sbfree(struct sockbuf *sb, struct mbuf *m);
extern void sbfree_chunk(struct sockbuf *sb, struct mbuf *m);

  
                     
   
#define SBL_WAIT        0x00000001                                              
#define SBL_NOINTR      0x00000002                                          
#define SBL_IGNDEFUNCT  0x00000004                                  
#define SBL_VALID       (SBL_WAIT | SBL_NOINTR | SBL_IGNDEFUNCT)
extern int sblock(struct sockbuf *sb, uint32_t flags);
extern void sbunlock(struct sockbuf *sb, boolean_t keeplocked);

extern int soaccept(struct socket *so, struct sockaddr **nam);
extern int soacceptlock(struct socket *so, struct sockaddr **nam, int dolock);
extern int soacceptfilter(struct socket *so, struct socket *head);
extern struct socket *soalloc(int waitok, int dom, int type);
extern int sobindlock(struct socket *so, struct sockaddr *nam, int dolock);
extern int soclose(struct socket *so);
extern int soclose_locked(struct socket *so);
extern void soclose_wait_locked(struct socket *so);
extern int soconnect(struct socket *so, struct sockaddr *nam);
extern int soconnectlock(struct socket *so, struct sockaddr *nam, int dolock);
extern int soconnect2(struct socket *so1, struct socket *so2);
extern int soconnectxlocked(struct socket *so, struct sockaddr *src,
    struct sockaddr *dst, struct proc *, uint32_t, sae_associd_t,
    sae_connid_t *, uint32_t, void *, u_int32_t, uio_t, user_ssize_t *);
extern int sodisconnectx(struct socket *so, sae_associd_t, sae_connid_t);
extern int sodisconnectxlocked(struct socket *so, sae_associd_t, sae_connid_t);
extern void soevupcall(struct socket *so, long hint);
                                 
#define SOCF_MPTCP      0x1                        
extern int socreate_internal(int dom, struct socket **aso, int type, int proto,
    struct proc *, uint32_t, struct proc *);
extern int socreate(int dom, struct socket **aso, int type, int proto);
extern int socreate_delegate(int dom, struct socket **aso, int type, int proto,
    pid_t epid);
extern void sodealloc(struct socket *so);
extern int sodisconnectlocked(struct socket *so);
extern void soreference(struct socket *so);
extern void sodereference(struct socket *so);
extern void somultipages(struct socket *, boolean_t);
extern void soif2kcl(struct socket *, boolean_t);
extern int sosetdefunct(struct proc *, struct socket *, int level, boolean_t);
extern int sodefunct(struct proc *, struct socket *, int level);
extern int soresume(struct proc *, struct socket *, int);
extern void resume_proc_sockets(proc_t);
extern int so_check_extended_bk_idle_time(struct socket *);
extern void so_drain_extended_bk_idle(struct socket *);
extern void sohasoutofband(struct socket *so);
extern void sodisconnectwakeup(struct socket *so);
extern int soisthrottled(struct socket *so);
extern int soisprivilegedtraffic(struct socket *so);
extern int soissrcbackground(struct socket *so);
extern int soissrcrealtime(struct socket *so);
extern int soissrcbesteffort(struct socket *so);
extern void soclearfastopen(struct socket *so);
extern int solisten(struct socket *so, int backlog);
extern struct socket *sodropablereq(struct socket *head);
extern lck_mtx_t *socket_getlock(struct socket *so, int flags);
extern void socket_lock(struct socket *so, int refcount);
extern void socket_lock_assert_owned(struct socket *so);
extern int socket_try_lock(struct socket *so);
extern void socket_unlock(struct socket *so, int refcount);
extern int sogetaddr_locked(struct socket *, struct sockaddr **, int);
extern const char *solockhistory_nr(struct socket *);
extern void soevent(struct socket *so, long hint);
extern void sorflush(struct socket *so);
extern void sowflush(struct socket *so);
extern void sowakeup(struct socket *so, struct sockbuf *sb, struct socket *so2);
extern int soioctl(struct socket *so, u_long cmd, caddr_t data, struct proc *p);
extern int sogetoptlock(struct socket *so, struct sockopt *sopt, int);
extern int sosetoptlock(struct socket *so, struct sockopt *sopt, int);
extern int soshutdown(struct socket *so, int how);
extern int soshutdownlock(struct socket *so, int how);
extern int soshutdownlock_final(struct socket *so, int how);
extern void sotoxsocket(struct socket *so, struct xsocket *xso);
#if XNU_TARGET_OS_OSX
extern void sotoxsocket64(struct socket *so, struct xsocket64 *xso);
#endif                        
extern int sosendallatonce(struct socket *so);
extern int soreadable(struct socket *so);
extern int sowriteable(struct socket *so);
extern void sowwakeup(struct socket *so);
extern int sosendcheck(struct socket *, struct sockaddr *, user_ssize_t,
    int32_t, int32_t, int, int *);

extern int soo_ioctl(struct fileproc *, u_long, caddr_t, vfs_context_t);
extern int soo_stat(struct socket *, void *, int);
extern int soo_select(struct fileproc *, int, void *, vfs_context_t);
extern int soo_kqfilter(struct fileproc *, struct knote *, struct kevent_qos_s *);

                                                                    
#define PKT_SCF_IPV6            0x00000001                       
#define PKT_SCF_TCP_ACK         0x00000002                        
#define PKT_SCF_TCP_SYN         0x00000004                   

  
                                                       
   

#define CONNREQF_MPTCP  0x1                                     
#define CONNREQF_UIO    0x2                       
#define CONNREQF_IDEM   0x4                             

extern void set_packet_service_class(struct mbuf *, struct socket *,
    mbuf_svc_class_t, u_int32_t);
extern void so_tc_update_stats(struct mbuf *, struct socket *,
    mbuf_svc_class_t);
extern int so_tos_from_control(struct mbuf *);
extern int so_tc_from_control(struct mbuf *, int *);
extern mbuf_svc_class_t so_tc2msc(int);
extern int so_svc2tc(mbuf_svc_class_t);

extern u_int8_t tcp_cansbgrow(struct sockbuf *sb);
extern void set_tcp_stream_priority(struct socket *so);

extern int so_set_net_service_type(struct socket *, int);
extern int so_set_traffic_class(struct socket *, int);
extern void so_set_default_traffic_class(struct socket *);
extern int so_set_opportunistic(struct socket *, int);
extern int so_get_opportunistic(struct socket *);
extern int so_set_recv_anyif(struct socket *, int);
extern int so_get_recv_anyif(struct socket *);
extern int so_set_effective_pid(struct socket *so, int epid, struct proc *p, boolean_t check_cred);
extern int so_set_effective_uuid(struct socket *so, uuid_t euuid, struct proc *p, boolean_t check_cred);
extern int so_set_restrictions(struct socket *, uint32_t);
extern uint32_t so_get_restrictions(struct socket *);
extern void socket_tclass_init(void);
#if (DEVELOPMENT || DEBUG)
extern int so_set_tcdbg(struct socket *, struct so_tcdbg *);
extern int sogetopt_tcdbg(struct socket *, struct sockopt *);
#endif                             

extern int so_isdstlocal(struct socket *);
extern void so_recv_data_stat(struct socket *, struct mbuf *, size_t);
extern void so_inc_recv_data_stat(struct socket *, size_t, size_t, uint32_t);
extern int so_wait_for_if_feedback(struct socket *);
extern int soopt_getm(struct sockopt *sopt, struct mbuf **mp);
extern int soopt_mcopyin(struct sockopt *sopt, struct mbuf *m);
extern int soopt_mcopyout(struct sockopt *sopt, struct mbuf *m);
extern boolean_t so_cache_timer(void);

extern void mptcp_fallback_sbdrop(struct socket *so, struct mbuf *m, int len);
extern void mptcp_preproc_sbdrop(struct socket *, struct mbuf *, unsigned int);
extern void mptcp_postproc_sbdrop(struct mbuf *, u_int64_t, u_int32_t,
    u_int32_t);

extern void netpolicy_post_msg(uint32_t, struct netpolicy_event_data *,
    uint32_t);

extern int tcp_notsent_lowat_check(struct socket *so);

extern user_ssize_t uio_array_resid(struct uio **, u_int);
extern user_ssize_t recv_msg_array_resid(struct recv_msg_elem *, u_int);

void sotoxsocket_n(struct socket *, struct xsocket_n *);
void sbtoxsockbuf_n(struct sockbuf *, struct xsockbuf_n *);
void sbtoxsockstat_n(struct socket *, struct xsockstat_n *);

__END_DECLS
#endif                         
#endif                     
#endif                         
