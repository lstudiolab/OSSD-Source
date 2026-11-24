/*
 * Copyright (c) 2003-2019 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
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
 *	$FreeBSD: src/sys/sys/event.h,v 1.5.2.5 2001/12/14 19:21:22 jlemon Exp $
 */

#ifndef _SYS_EVENT_H_
#define _SYS_EVENT_H_

#include <machine/types.h>
#include <sys/cdefs.h>
#include <stdint.h>

  
               
   
#define EVFILT_READ             (-1)
#define EVFILT_WRITE            (-2)
#define EVFILT_AIO              (-3)                                  
#define EVFILT_VNODE            (-4)                            
#define EVFILT_PROC             (-5)                                 
#define EVFILT_SIGNAL           (-6)                                 
#define EVFILT_TIMER            (-7)                
#define EVFILT_MACHPORT         (-8)                       
#define EVFILT_FS               (-9)                           
#define EVFILT_USER             (-10)                    
#ifdef PRIVATE
#define EVFILT_UNUSED_11        (-11)                     
#endif
#define EVFILT_VM               (-12)                              
#ifdef PRIVATE
#define EVFILT_SOCK             (-13)                      
#define EVFILT_MEMORYSTATUS     (-14)                            
#endif              
#define EVFILT_EXCEPT           (-15)                         
#ifdef PRIVATE
#define EVFILT_WORKLOOP         (-17)                        
#endif              

#define EVFILT_SYSCOUNT         17
#define EVFILT_THREADMARKER     EVFILT_SYSCOUNT                        

#pragma pack(4)

struct kevent {
	uintptr_t       ident;                                 
	int16_t         filter;                       
	uint16_t        flags;                     
	uint32_t        fflags;                            
	intptr_t        data;                             
	void            *udata;                                  
};

#ifdef KERNEL_PRIVATE

struct user64_kevent {
	uint64_t        ident;                                 
	int16_t         filter;                       
	uint16_t        flags;                     
	uint32_t        fflags;                            
	int64_t         data;                             
	user_addr_t     udata;                                   
};

struct user32_kevent {
	uint32_t        ident;                                 
	int16_t         filter;                       
	uint16_t        flags;                     
	uint32_t        fflags;                            
	int32_t         data;                             
	user32_addr_t   udata;                                   
};

#endif                     

#pragma pack()

struct kevent64_s {
	uint64_t        ident;                                         
	int16_t         filter;                               
	uint16_t        flags;                             
	uint32_t        fflags;                                    
	int64_t         data;                                     
	uint64_t        udata;                                           
	uint64_t        ext[2];                                         
};

#ifdef PRIVATE
struct kevent_qos_s {
	uint64_t        ident;                                         
	int16_t         filter;                               
	uint16_t        flags;                             
	int32_t         qos;                                    
	uint64_t        udata;                                           
	uint32_t        fflags;                                    
	uint32_t        xflags;                                          
	int64_t         data;                                     
	uint64_t        ext[4];                                         
};

  
                                                                  
   
typedef uint64_t kqueue_id_t;
#endif              

#define EV_SET(kevp, a, b, c, d, e, f) do {     \
	struct kevent *__kevp__ = (kevp);       \
	__kevp__->ident = (a);                  \
	__kevp__->filter = (b);                 \
	__kevp__->flags = (c);                  \
	__kevp__->fflags = (d);                 \
	__kevp__->data = (e);                   \
	__kevp__->udata = (f);                  \
} while(0)

#define EV_SET64(kevp, a, b, c, d, e, f, g, h) do {     \
	struct kevent64_s *__kevp__ = (kevp);           \
	__kevp__->ident = (a);                          \
	__kevp__->filter = (b);                         \
	__kevp__->flags = (c);                          \
	__kevp__->fflags = (d);                         \
	__kevp__->data = (e);                           \
	__kevp__->udata = (f);                          \
	__kevp__->ext[0] = (g);                         \
	__kevp__->ext[1] = (h);                         \
} while(0)


                              
#define KEVENT_FLAG_NONE                         0x000000                          
#define KEVENT_FLAG_IMMEDIATE                    0x000001                              
#define KEVENT_FLAG_ERROR_EVENTS                 0x000002                                                     

#ifdef PRIVATE

  
                                                                     
                                                                         
           
   

                                                         
#define KEVENT_FLAG_STACK_DATA                   0x000008                                                    
                                                         
#define KEVENT_FLAG_WORKQ                        0x000020                                           
                                                         
                                                         
                                                         
#define KEVENT_FLAG_WORKQ_MANAGER                0x000200                 
#define KEVENT_FLAG_WORKLOOP                     0x000400                                                
#define KEVENT_FLAG_PARKING                      0x000800                                
                                                         
                                                         
                                                         
#define KEVENT_FLAG_WORKLOOP_SERVICER_ATTACH     0x008000                 
#define KEVENT_FLAG_WORKLOOP_SERVICER_DETACH     0x010000                 
#define KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST        0x020000                                   
#define KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST    0x040000                                       
#define KEVENT_FLAG_WORKLOOP_NO_WQ_THREAD        0x080000                 

#ifdef XNU_KERNEL_PRIVATE

#define KEVENT_FLAG_POLL                         0x0010                          
#define KEVENT_FLAG_LEGACY32                     0x0040                                          
#define KEVENT_FLAG_LEGACY64                     0x0080                                          
#define KEVENT_FLAG_PROC64                       0x0100                      
#define KEVENT_FLAG_KERNEL                       0x1000                           
#define KEVENT_FLAG_DYNAMIC_KQUEUE               0x2000                                       
#define KEVENT_FLAG_NEEDS_END_PROCESSING         0x4000                                                

#define KEVENT_ID_FLAG_USER (KEVENT_FLAG_WORKLOOP | \
	        KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST | KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST)

#define KEVENT_FLAG_USER (KEVENT_FLAG_IMMEDIATE | KEVENT_FLAG_ERROR_EVENTS | \
	        KEVENT_FLAG_STACK_DATA | KEVENT_FLAG_WORKQ | KEVENT_FLAG_WORKLOOP | \
	        KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST | KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST)

  
                                                                         
                                                                               
                                                    
   
#define EVFILTID_KQREAD            (EVFILT_SYSCOUNT)
#define EVFILTID_PIPE_N            (EVFILT_SYSCOUNT + 1)
#define EVFILTID_PIPE_R            (EVFILT_SYSCOUNT + 2)
#define EVFILTID_PIPE_W            (EVFILT_SYSCOUNT + 3)
#define EVFILTID_PTSD              (EVFILT_SYSCOUNT + 4)
#define EVFILTID_SOREAD            (EVFILT_SYSCOUNT + 5)
#define EVFILTID_SOWRITE           (EVFILT_SYSCOUNT + 6)
#define EVFILTID_SCK               (EVFILT_SYSCOUNT + 7)
#define EVFILTID_SOEXCEPT          (EVFILT_SYSCOUNT + 8)
#define EVFILTID_SPEC              (EVFILT_SYSCOUNT + 9)
#define EVFILTID_BPFREAD           (EVFILT_SYSCOUNT + 10)
#define EVFILTID_NECP_FD           (EVFILT_SYSCOUNT + 11)
#define EVFILTID_FSEVENT           (EVFILT_SYSCOUNT + 15)
#define EVFILTID_VN                (EVFILT_SYSCOUNT + 16)
#define EVFILTID_TTY               (EVFILT_SYSCOUNT + 17)
#define EVFILTID_PTMX              (EVFILT_SYSCOUNT + 18)

#define EVFILTID_DETACHED          (EVFILT_SYSCOUNT + 19)
#define EVFILTID_MAX               (EVFILT_SYSCOUNT + 20)

#endif                                  

#define EV_SET_QOS 0

#endif              

             
#define EV_ADD              0x0001                                            
#define EV_DELETE           0x0002                                
#define EV_ENABLE           0x0004                        
#define EV_DISABLE          0x0008                                        

           
#define EV_ONESHOT          0x0010                                      
#define EV_CLEAR            0x0020                                             
#define EV_RECEIPT          0x0040                                        
                                                                          
                                                                              
                                                                              

#define EV_DISPATCH         0x0080                                         
#define EV_UDATA_SPECIFIC   0x0100                                         

#define EV_DISPATCH2        (EV_DISPATCH | EV_UDATA_SPECIFIC)
                                       
                                            
                                        
                                       

#define EV_VANISHED         0x0200                                            
                                                                              

#define EV_SYSFLAGS         0xF000                              
#define EV_FLAG0            0x1000                                
#define EV_FLAG1            0x2000                                

                     
#define EV_EOF              0x8000                        
#define EV_ERROR            0x4000                                      

  
                                        
  
                                                                           
                                                        
  
                                                                          
                                                                              
                                                       
  
                                                                          
                                                                        
                                                                     
                                                                            
                  
                                                                                   
                                                                                  
                                                                                   
                                                                            
  
                                                                             
                                                                            
                                                                               
                              
   
#define EV_POLL         EV_FLAG0
#define EV_OOBAND       EV_FLAG1

  
                                                          
   

  
                                                                      
   
#define NOTE_TRIGGER    0x01000000

  
                                                                           
                                                        
  
                                                                       
                                                                   
   
#define NOTE_FFNOP      0x00000000                                       
#define NOTE_FFAND      0x40000000                              
#define NOTE_FFOR       0x80000000                             
#define NOTE_FFCOPY     0xc0000000                               
#define NOTE_FFCTRLMASK 0xc0000000                                       
#define NOTE_FFLAGSMASK 0x00ffffff

#ifdef PRIVATE
  
                                                              
  
                                                                         
                                                                                   
  
  
            
  
                                         
                                                                            
                                                                    
  
                                    
                                                                                
                                                                              
                                  
  
                                                                           
  
                                    
                                                              
                                                                          
  
                                   
                                                     
  
                   
  
                                                               
                                                                               
                        
  
                                    
                                                                           
                                                                                
                               
  
                                     
                                                                              
                                                                               
                                                                                
                                     
  
                                    
                                                                          
                                                                             
        
                                          
                                               
                                                                     
  
                                                      
   
#define NOTE_WL_THREAD_REQUEST   0x00000001
#define NOTE_WL_SYNC_WAIT        0x00000004
#define NOTE_WL_SYNC_WAKE        0x00000008
#define NOTE_WL_SYNC_IPC         0x80000000
#define NOTE_WL_COMMANDS_MASK    0x8000000f                                          

#define NOTE_WL_UPDATE_QOS       0x00000010
#define NOTE_WL_END_OWNERSHIP    0x00000020
#define NOTE_WL_DISCOVER_OWNER   0x00000080
#define NOTE_WL_IGNORE_ESTALE    0x00000100
#define NOTE_WL_UPDATES_MASK     0x000001f0                                         

#define NOTE_WL_UPDATE_OWNER     0                                   

  
                                                
   
#define EV_EXTIDX_WL_LANE        0                                              
                                                                                    
#define EV_EXTIDX_WL_ADDR        1                                                         
#define EV_EXTIDX_WL_MASK        2                                                         
#define EV_EXTIDX_WL_VALUE       3                                                      
                                                                                    

#endif              

  
                                                                  
  
                                                                    
                                                        
   
#define NOTE_LOWAT      0x00000001                                  

                                                              
#define NOTE_OOB        0x00000002                            

  
                                                           
   
#define NOTE_DELETE     0x00000001                                     
#define NOTE_WRITE      0x00000002                                         
#define NOTE_EXTEND     0x00000004                                  
#define NOTE_ATTRIB     0x00000008                                      
#define NOTE_LINK       0x00000010                                      
#define NOTE_RENAME     0x00000020                                     
#define NOTE_REVOKE     0x00000040                                            
#define NOTE_NONE       0x00000080                                                                              
#define NOTE_FUNLOCK    0x00000100                                                  

  
                                                          
  
                                                                           
                                                                             
                                                                             
                                                                              
                                                                           
                      
   
enum {
	eNoteReapDeprecated __deprecated_enum_msg("This kqueue(2) EVFILT_PROC flag is deprecated") = 0x10000000
};

#define NOTE_EXIT               0x80000000                          
#define NOTE_FORK               0x40000000                          
#define NOTE_EXEC               0x20000000                          
#define NOTE_REAP               ((unsigned int)eNoteReapDeprecated                  )                       
#define NOTE_SIGNAL             0x08000000                                     
#define NOTE_EXITSTATUS         0x04000000                                                                                                    
#define NOTE_EXIT_DETAIL        0x02000000                                               

#define NOTE_PDATAMASK  0x000fffff                                                 
#define NOTE_PCTRLMASK  (~NOTE_PDATAMASK)

  
                                                                                
   
enum {
	eNoteExitReparentedDeprecated __deprecated_enum_msg("This kqueue(2) EVFILT_PROC flag is no longer sent") = 0x00080000
};
#define NOTE_EXIT_REPARENTED    ((unsigned int)eNoteExitReparentedDeprecated)                                

  
                                                                                    
   
#define NOTE_EXIT_DETAIL_MASK           0x00070000
#define NOTE_EXIT_DECRYPTFAIL           0x00010000
#define NOTE_EXIT_MEMORY                0x00020000
#define NOTE_EXIT_CSERROR               0x00040000

#ifdef PRIVATE

  
                                                                                 
   
#define NOTE_EXIT_MEMORY_DETAIL_MASK    0xfe000000
#define NOTE_EXIT_MEMORY_VMPAGESHORTAGE 0x80000000                                                                                        
#define NOTE_EXIT_MEMORY_VMTHRASHING    0x40000000                                                                                    
#define NOTE_EXIT_MEMORY_HIWAT          0x20000000                                                                 
#define NOTE_EXIT_MEMORY_PID            0x10000000                                                        
#define NOTE_EXIT_MEMORY_IDLE           0x08000000                                                     
#define NOTE_EXIT_MEMORY_VNODE          0X04000000                                               
#define NOTE_EXIT_MEMORY_FCTHRASHING    0x02000000                                                                                           

#endif

  
                                                         
   
#define NOTE_VM_PRESSURE                        0x80000000                                                 
#define NOTE_VM_PRESSURE_TERMINATE              0x40000000                                                                                        
#define NOTE_VM_PRESSURE_SUDDEN_TERMINATE       0x20000000                                                            
#define NOTE_VM_ERROR                           0x10000000                                      

#ifdef PRIVATE

  
                                                                   
   
#define NOTE_MEMORYSTATUS_PRESSURE_NORMAL       0x00000001                                                         
#define NOTE_MEMORYSTATUS_PRESSURE_WARN         0x00000002                                                                   
#define NOTE_MEMORYSTATUS_PRESSURE_CRITICAL     0x00000004                                                                    
#define NOTE_MEMORYSTATUS_LOW_SWAP              0x00000008                                         
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN       0x00000010                                                        
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL   0x00000020                                                                      
#define NOTE_MEMORYSTATUS_MSL_STATUS   0xf0000000                                                             

#ifdef KERNEL_PRIVATE
  
                                                                           
   
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN_ACTIVE        0x00000040                                                                                               
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN_INACTIVE      0x00000080                                                                                                
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL_ACTIVE    0x00000100                                                                                                 
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL_INACTIVE  0x00000200                                                                                                   
#define NOTE_MEMORYSTATUS_JETSAM_FG_BAND                0x00000400                                                 

  
                                                     
   
#define EVFILT_MEMORYSTATUS_ALL_MASK \
	(NOTE_MEMORYSTATUS_PRESSURE_NORMAL | NOTE_MEMORYSTATUS_PRESSURE_WARN | NOTE_MEMORYSTATUS_PRESSURE_CRITICAL | NOTE_MEMORYSTATUS_LOW_SWAP | \
	 NOTE_MEMORYSTATUS_PROC_LIMIT_WARN | NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL | NOTE_MEMORYSTATUS_MSL_STATUS)

#endif                     

typedef enum vm_pressure_level {
	kVMPressureNormal   = 0,
	kVMPressureWarning  = 1,
	kVMPressureUrgent   = 2,
	kVMPressureCritical = 3,
	kVMPressureJetsam   = 4,                                   
} vm_pressure_level_t;

#endif              

  
                                                            
                                                            
                                                   
  
                                               
   
#define NOTE_SECONDS    0x00000001                                           
#define NOTE_USECONDS   0x00000002                                           
#define NOTE_NSECONDS   0x00000004                                           
#define NOTE_ABSOLUTE   0x00000008                                           
                                                                  
#define NOTE_LEEWAY             0x00000010                                                              
#define NOTE_CRITICAL   0x00000020                                                        
#define NOTE_BACKGROUND 0x00000040                                                        
#define NOTE_MACH_CONTINUOUS_TIME       0x00000080
  
                             
                                                                         
                                     
                                                                        
                                                                              
   
#define NOTE_MACHTIME   0x00000100                                                    
                                               

#ifdef PRIVATE
  
                                                           
  
   
#define NOTE_CONNRESET          0x00000001                   
#define NOTE_READCLOSED         0x00000002                            
#define NOTE_WRITECLOSED        0x00000004                             
#define NOTE_TIMEOUT            0x00000008                                            
#define NOTE_NOSRCADDR          0x00000010                                   
#define NOTE_IFDENIED           0x00000020                                  
#define NOTE_SUSPEND            0x00000040                             
#define NOTE_RESUME             0x00000080                           
#define NOTE_KEEPALIVE          0x00000100                             
#define NOTE_ADAPTIVE_WTIMO     0x00000200                                 
#define NOTE_ADAPTIVE_RTIMO     0x00000400                                
#define NOTE_CONNECTED          0x00000800                          
#define NOTE_DISCONNECTED       0x00001000                             
#define NOTE_CONNINFO_UPDATED   0x00002000                                  
#define NOTE_NOTIFY_ACK         0x00004000                             

#define EVFILT_SOCK_LEVEL_TRIGGER_MASK \
	        (NOTE_READCLOSED | NOTE_WRITECLOSED | NOTE_SUSPEND | NOTE_RESUME | \
	         NOTE_CONNECTED | NOTE_DISCONNECTED)

#define EVFILT_SOCK_ALL_MASK \
	        (NOTE_CONNRESET | NOTE_READCLOSED | NOTE_WRITECLOSED | NOTE_TIMEOUT | \
	        NOTE_NOSRCADDR | NOTE_IFDENIED | NOTE_SUSPEND | NOTE_RESUME | \
	        NOTE_KEEPALIVE | NOTE_ADAPTIVE_WTIMO | NOTE_ADAPTIVE_RTIMO | \
	        NOTE_CONNECTED | NOTE_DISCONNECTED | NOTE_CONNINFO_UPDATED | \
	        NOTE_NOTIFY_ACK)

#endif              

  
                                                               
  
                                            
  
                                                                            
                                                                      
                                                                                
                                                                                       
                                                                                     
                                                                                    
                                                                                 
                                                                                     
                                                                             
  
                                                                                  
                                                                                   
                                                                        
                                                                                   
                                                                           
                                                                                       
  
                                                                                   
                                                                                 
                                                                                  
                                                                              
                                                                    
  
                                                                               
                                                                              
                                                                        
   

  
                      
                                                                               
   
                                      
#define NOTE_TRACK      0x00000001                                       
#define NOTE_TRACKERR   0x00000002                                         
#define NOTE_CHILD      0x00000004                                      


#ifdef PRIVATE
#endif              

#ifndef KERNEL
                                                                               
#include <sys/queue.h>
struct knote;
SLIST_HEAD(klist, knote);
#endif

#ifdef KERNEL

#ifdef XNU_KERNEL_PRIVATE
#include <sys/queue.h>
#include <mach/vm_param.h>
#include <kern/kern_types.h>
#include <sys/fcntl.h>                    
#include <kern/debug.h>            
#include <pthread/priority_private.h>

LIST_HEAD(knote_list, knote);
TAILQ_HEAD(kqtailq, knote);                                    

                                  
typedef uint8_t kq_index_t;

                                   
__options_decl(kn_status_t, uint16_t                     , {
	KN_ACTIVE         = 0x001,                                
	KN_QUEUED         = 0x002,                         
	KN_DISABLED       = 0x004,                         
	KN_DROPPING       = 0x008,                              
	KN_LOCKED         = 0x010,                                    
	KN_POSTING        = 0x020,                           
	KN_STAYACTIVE     = 0x040,                                  
	KN_DEFERDELETE    = 0x080,                                     
	KN_MERGE_QOS      = 0x100,                                                                 
	KN_REQVANISH      = 0x200,                           
	KN_VANISHED       = 0x400,                    
	KN_SUPPRESSED     = 0x800,                                           
});

#if __LP64__
#define KNOTE_KQ_PACKED_BITS   42
#define KNOTE_KQ_PACKED_SHIFT   0
#define KNOTE_KQ_PACKED_BASE    0
#else
#define KNOTE_KQ_PACKED_BITS   32
#define KNOTE_KQ_PACKED_SHIFT   0
#define KNOTE_KQ_PACKED_BASE    0
#endif

_Static_assert(!VM_PACKING_IS_BASE_RELATIVE(KNOTE_KQ_PACKED),
    "Make sure the knote pointer packing is based on arithmetic shifts");

struct kqueue;
struct knote {
	TAILQ_ENTRY(knote)       kn_tqe;                                        
	SLIST_ENTRY(knote)       kn_link;                                        
	SLIST_ENTRY(knote)       kn_selnext;                                 

	kn_status_t              kn_status : 12;
	uintptr_t
	    kn_qos_index:4,                                               
	    kn_qos_override:3,                                              
	    kn_is_fd:1,                                                 
	    kn_vnode_kqok:1,
	    kn_vnode_use_ofst:1;
#if __LP64__
	uintptr_t                   kn_kq_packed : KNOTE_KQ_PACKED_BITS;
#else
	uintptr_t                   kn_kq_packed;
#endif

	                                                              
	union {
		void               *kn_hook;
		uint32_t            kn_hook32;
#if __LP64__
		uint64_t            kn_hook_waitqid;
#else
		uint32_t            kn_hook_waitqid;
#endif
	};

	                                                      
	union {
		struct fileproc    *kn_fp;                                 
		struct proc        *kn_proc;                          
		struct ipc_mqueue  *kn_mqueue;                        
		struct thread_call *kn_thcall;
		struct thread      *kn_thread;
	};

	  
                                                                  
                        
   
                                                                        
                                                
                                                                
   
                                                                   
                                              
   
                                                                        
                                                                      
                                   
   
                                                          
                                  
    
	struct kevent_internal_s {
		uint64_t    kei_ident;                                     
#ifdef __LITTLE_ENDIAN__
		int8_t      kei_filter;                           
		uint8_t     kei_filtid;                                  
#else
		uint8_t     kei_filtid;                                  
		int8_t      kei_filter;                           
#endif
		uint16_t    kei_flags;                         
		int32_t     kei_qos;                                
		uint64_t    kei_udata;                                       
		uint32_t    kei_fflags;                                
		uint32_t    kei_sfflags;                             
		int64_t     kei_sdata;                                             
		uint64_t    kei_ext[4];                                     
	} kn_kevent;

#define kn_id           kn_kevent.kei_ident
#define kn_filtid       kn_kevent.kei_filtid
#define kn_filter       kn_kevent.kei_filter
#define kn_flags        kn_kevent.kei_flags
#define kn_qos          kn_kevent.kei_qos
#define kn_udata        kn_kevent.kei_udata
#define kn_fflags       kn_kevent.kei_fflags
#define kn_sfflags      kn_kevent.kei_sfflags
#define kn_sdata        kn_kevent.kei_sdata
#define kn_ext          kn_kevent.kei_ext
};

static inline struct kqueue *
knote_get_kq(struct knote *kn)
{
	return (struct kqueue *)VM_UNPACK_POINTER(kn->kn_kq_packed, KNOTE_KQ_PACKED);
}

static inline int
knote_get_seltype(struct knote *kn)
{
	switch (kn->kn_filter) {
	case EVFILT_READ:
		return FREAD;
	case EVFILT_WRITE:
		return FWRITE;
	default:
		panic("%s(%p): invalid filter %d\n",
		    __func__, kn, kn->kn_filter);
		return 0;
	}
}

struct kevent_ctx_s {
	uint64_t         kec_data_avail;                                        
	user_addr_t      kec_data_out;                              
	user_size_t      kec_data_size;                                
	user_size_t      kec_data_resid;                                  
	uint64_t         kec_deadline;                                                      
	struct fileproc *kec_fp;                                                     
	int              kec_fd;                                             

	                                                         
	int              kec_process_nevents;                                   
	int              kec_process_noutputs;                                   
	unsigned int     kec_process_flags;                                                  
	user_addr_t      kec_process_eventlist;                                        
};
typedef struct kevent_ctx_s *kevent_ctx_t;

kevent_ctx_t
kevent_get_context(thread_t thread);

  
                   
  
                                                                                         
                                                                
  
                                                                               
                                                              
  
                                                                         
                                                                           
                                         
  
                                                                           
                                              
  
  
                         
  
           
                                                                                       
  
                                                                                     
                                                                                          
                                                                                          
                  
  
                                                                                    
  
                  
                                                                          
  
                                                                                
                                                                               
  
                                                                            
  
                     
                                                                           
                                                   
  
             
                                                                                            
                                                                                    
  
                                                                                      
                                                                                       
                                                                                     
                                                                                               
                                                                                                 
                                                                                               
                                                                       
  
                                                                                              
                                                                                             
                                   
  
                                                                                             
                                                                                      
  
             
                                                                                       
                                                                                       
                                                     
  
            
                                                                                        
                                                                 
  
                                                                                      
                                                                                       
                        
  
                                                                                             
                                                                                      
  
              
                                                                           
  
                                                                                    
                                                                                     
                                                                                      
                                                                                  
                                                                                    
  
                                                                               
                                                                                 
                                                                                  
                                                                 
  
                                                                                         
                                                           
  
                                                                                 
                                                                                   
                                                                                 
                                                                                 
                                                                                  
                                                    
  
                                                                                
                                                                                
                                                                                      
  
                                                                                
                                                                                     
                                                                                
                                                                                      
                                                              
  
                                                                                 
                                                                                    
                                                        
  
            
                                                                        
                                                                 
  
                                                                               
  
                                                                   
  
                                                                               
                      
  
                                                                           
                                                                             
                                                         
  
           
                                                                                
                                                                             
  
                 
  
                                                                             
                                                                         
                                                                              
                      
  
                                                                              
  
                                                                                
                                                      
  
                                                                            
  
                         
                                                                                
                                                                              
                                         
  
                                                                            
   

struct _kevent_register;
struct knote_lock_ctx;
struct proc;
struct uthread;
struct waitq;

struct filterops {
	bool    f_isfd;                                                    
	bool    f_adjusts_qos;                                                   
	bool    f_extended_codes;                                  

	int     (*f_attach)(struct knote *kn, struct kevent_qos_s *kev);
	void    (*f_detach)(struct knote *kn);
	int     (*f_event)(struct knote *kn, long hint);
	int     (*f_touch)(struct knote *kn, struct kevent_qos_s *kev);
	int     (*f_process)(struct knote *kn, struct kevent_qos_s *kev);
	int     (*f_peek)(struct knote *kn);

	                         
	bool    (*f_allow_drop)(struct knote *kn, struct kevent_qos_s *kev);
	void    (*f_post_register_wait)(struct uthread *uth, struct knote *kn,
	    struct _kevent_register *ss_kr);
};

  
                                                                           
  
                
                                                                          
  
                                                                           
                                                                         
                                                                            
  
                                                              
                  
                  
  
                       
                                                                
  
                                                                             
                                                              
  
                                  
                  
                                           
  
                        
                                                                                
                                                                              
  
                                                                            
                                                        
  
                        
                         
                                           
  
                         
                               
                                                                           
                                                                               
                                                                
  
                                                                         
                                                                          
  
                                                                      
                                                                             
                                
  
                                                                              
  
                                                                            
                                                                             
                                                                             
                                               
  
                                                      
                  
                       
  
                            
                                                                           
                                                                           
                                                
  
  
                                  
                  
                                           
   
#define FILTER_ACTIVE                       0x00000001
#define FILTER_REGISTER_WAIT                0x00000002
#define FILTER_UPDATE_REQ_QOS               0x00000004
#define FILTER_ADJUST_EVENT_QOS_BIT         0x00000008
#define FILTER_ADJUST_EVENT_QOS_MASK        0x00000070
#define FILTER_ADJUST_EVENT_QOS_SHIFT 4
#define FILTER_ADJUST_EVENT_QOS(qos) \
	        (((qos) << FILTER_ADJUST_EVENT_QOS_SHIFT) | FILTER_ADJUST_EVENT_QOS_BIT)
#define FILTER_RESET_EVENT_QOS              FILTER_ADJUST_EVENT_QOS_BIT
#define FILTER_THREADREQ_NODEFEER           0x00000080

#define filter_call(_ops, call)  \
	        ((_ops)->f_extended_codes ? (_ops)->call : !!((_ops)->call))

SLIST_HEAD(klist, knote);
extern void     knote_init(void);
extern void     klist_init(struct klist *list);

#define KNOTE(list, hint)       knote(list, hint)
#define KNOTE_ATTACH(list, kn)  knote_attach(list, kn)
#define KNOTE_DETACH(list, kn)  knote_detach(list, kn)

extern void knote(struct klist *list, long hint);
extern int knote_attach(struct klist *list, struct knote *kn);
extern int knote_detach(struct klist *list, struct knote *kn);
extern void knote_vanish(struct klist *list, bool make_active);

extern void knote_set_error(struct knote *kn, int error);
extern int64_t knote_low_watermark(const struct knote *kn) __pure2;
extern void knote_fill_kevent_with_sdata(struct knote *kn, struct kevent_qos_s *kev);
extern void knote_fill_kevent(struct knote *kn, struct kevent_qos_s *kev, int64_t data);

extern void knote_link_waitqset_lazy_alloc(struct knote *kn);
extern boolean_t knote_link_waitqset_should_lazy_alloc(struct knote *kn);
extern int knote_link_waitq(struct knote *kn, struct waitq *wq, uint64_t *reserved_link);
extern int knote_unlink_waitq(struct knote *kn, struct waitq *wq);
extern void knote_fdclose(struct proc *p, int fd);
extern void knote_markstayactive(struct knote *kn);
extern void knote_clearstayactive(struct knote *kn);
extern const struct filterops *knote_fops(struct knote *kn);

extern struct turnstile *kqueue_turnstile(struct kqueue *);
extern struct turnstile *kqueue_alloc_turnstile(struct kqueue *);

int kevent_proc_copy_uptrs(void *proc, uint64_t *buf, uint32_t bufsize);
int kevent_copyout_proc_dynkqids(void *proc, user_addr_t ubuf,
    uint32_t ubufsize, int32_t *nkqueues_out);
int kevent_copyout_dynkqinfo(void *proc, kqueue_id_t kq_id, user_addr_t ubuf,
    uint32_t ubufsize, int32_t *size_out);
int kevent_copyout_dynkqextinfo(void *proc, kqueue_id_t kq_id, user_addr_t ubuf,
    uint32_t ubufsize, int32_t *nknotes_out);

extern int filt_wlattach_sync_ipc(struct knote *kn);
extern void filt_wldetach_sync_ipc(struct knote *kn);

extern int kevent_workq_internal(struct proc *p,
    user_addr_t changelist, int nchanges,
    user_addr_t eventlist, int nevents,
    user_addr_t data_out, user_size_t *data_available,
    unsigned int flags, int32_t *retval);

#elif defined(KERNEL_PRIVATE)                                                                         

#include <sys/queue.h>
struct proc;
struct knote;
SLIST_HEAD(klist, knote);

#endif                                            

#else               

#include <sys/types.h>

struct timespec;

__BEGIN_DECLS
int     kqueue(void);
int     kevent(int kq,
    const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents,
    const struct timespec *timeout);
int     kevent64(int kq,
    const struct kevent64_s *changelist, int nchanges,
    struct kevent64_s *eventlist, int nevents,
    unsigned int flags,
    const struct timespec *timeout);

#ifdef PRIVATE
int     kevent_qos(int kq,
    const struct kevent_qos_s *changelist, int nchanges,
    struct kevent_qos_s *eventlist, int nevents,
    void *data_out, size_t *data_available,
    unsigned int flags);

int     kevent_id(kqueue_id_t id,
    const struct kevent_qos_s *changelist, int nchanges,
    struct kevent_qos_s *eventlist, int nevents,
    void *data_out, size_t *data_available,
    unsigned int flags);
#endif              

__END_DECLS


#endif             

#ifdef PRIVATE

                                                                          
#define R2K_WORKLOOP_PENDING_EVENTS             0x1
#define R2K_WORKQ_PENDING_EVENTS                0x2

#endif              

#endif                     
