/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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

#ifndef _KDP_PROTOCOL_H_
#define _KDP_PROTOCOL_H_

  
                                          
   


#ifdef MACH_KERNEL_PRIVATE
#include        <mach/vm_prot.h>
#include    <mach/boolean.h>
#include        <stdint.h>
#endif

#ifdef KDP_PROXY_PACK_SUPPORT
#pragma pack(1)
#define KDP_PACKED
#else
#define KDP_PACKED __attribute__((packed))
#endif

  
                        
   
#if     DDEBUG_DEBUG || DEBUG_DEBUG
#define KDP_REXMIT_SECS         20                                      
#else                                    
#define KDP_REXMIT_SECS         3                                       
#endif                                   
#define KDP_REXMIT_TRIES        8                                       


  
                                
                                                                 
                                                        
   
#define KDP_MAX_ATTN_WAIT       30                                  

  
                                      
                                                                          
   
#define KDP_REMOTE_PORT         41139                                 

  
                                                                          
                                                                
   
#define UDP_HOST_COMM_BASE      41140
#define UDP_HOST_EXCEP_BASE     41145
#define NUM_UDP_HOST_PORTS      5

  
           
   
typedef enum {
	                                  
	KDP_CONNECT, KDP_DISCONNECT,

	                           
	KDP_HOSTINFO, KDP_VERSION, KDP_MAXBYTES,

	                   
	KDP_READMEM, KDP_WRITEMEM,

	                     
	KDP_READREGS, KDP_WRITEREGS,

	                           
	KDP_LOAD, KDP_IMAGEPATH,

	                       
	KDP_SUSPEND, KDP_RESUMECPUS,

	                                                               
	KDP_EXCEPTION, KDP_TERMINATION,

	                        
	KDP_BREAKPOINT_SET, KDP_BREAKPOINT_REMOVE,

	                
	KDP_REGIONS,

	                                  
	KDP_REATTACH,

	                           
	KDP_HOSTREBOOT,

	                                                                
	KDP_READMEM64, KDP_WRITEMEM64,

	                                                                     
	KDP_BREAKPOINT64_SET, KDP_BREAKPOINT64_REMOVE,

	                                                                     
	KDP_KERNELVERSION,

	                                                                         
	KDP_READPHYSMEM64, KDP_WRITEPHYSMEM64,

	                                         
	KDP_READIOPORT, KDP_WRITEIOPORT,

	                         
	KDP_READMSR64, KDP_WRITEMSR64,

	                                  
	KDP_DUMPINFO,

	                    
	KDP_INVALID_REQUEST
} kdp_req_t;

typedef enum {
	KDP_DUMPINFO_GETINFO    = 0x00000000,
	KDP_DUMPINFO_SETINFO    = 0x00000001,
	KDP_DUMPINFO_CORE       = 0x00000102,
	KDP_DUMPINFO_PANICLOG   = 0x00000103,
	KDP_DUMPINFO_SYSTEMLOG  = 0x00000104,
	KDP_DUMPINFO_DISABLE    = 0x00000105,
	KDP_DUMPINFO_MASK       = 0x00000FFF,
	KDP_DUMPINFO_DUMP       = 0x00000100,

	KDP_DUMPINFO_REBOOT     = 0x10000000,
	KDP_DUMPINFO_NORESUME   = 0x20000000,
	KDP_DUMPINFO_RESUME     = 0x00000000,                        
	KDP_DUMPINFO_NOINTR     = 0x40000000,                      
	KDP_DUMPINFO_INTR       = 0x00000000,                        
} kdp_dumpinfo_t;

  
                           
                                                                                         
                                                                                     
                                                  
   
typedef struct {
	kdp_req_t       request:7;                                   
	unsigned        is_reply:1;                                   
	unsigned        seq:8;                                              
	unsigned        len:16;                                                 
	unsigned        key;                             
} KDP_PACKED kdp_hdr_t;

  
             
   
typedef enum {
	KDPERR_NO_ERROR = 0,
	KDPERR_ALREADY_CONNECTED,
	KDPERR_BAD_NBYTES,
	KDPERR_BADFLAVOR,                                           
	KDPERR_BAD_ACCESS,                                            

	KDPERR_MAX_BREAKPOINTS = 100,
	KDPERR_BREAKPOINT_NOT_FOUND = 101,
	KDPERR_BREAKPOINT_ALREADY_SET = 102
} kdp_error_t;

#if defined(__x86_64__)
#define KDPERR_ACCESS(_req, _ret)  \
	(((_req) == (uint32_t)(_ret)) ? KDPERR_NO_ERROR : KDPERR_BAD_ACCESS)
#else
#define KDPERR_ACCESS(req, cnt)  (KDPERR_NO_ERROR)
#endif             


  
                                        
   

  
              
   
typedef struct {                                                 
	kdp_hdr_t       hdr;
	uint16_t        req_reply_port;                                     
	uint16_t        exc_note_port;                                        
	char            greeting[0];                                     
} KDP_PACKED kdp_connect_req_t;

typedef struct {                                               
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_connect_reply_t;

  
                 
   
typedef struct {                                                    
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_disconnect_req_t;

typedef struct {                                                  
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_disconnect_reply_t;

  
               
   
typedef struct {
	kdp_hdr_t hdr;
	uint16_t req_reply_port;                                     
} KDP_PACKED kdp_reattach_req_t;

  
               
   
typedef struct {                                                  
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_hostinfo_req_t;

typedef struct {
	uint32_t                cpus_mask;                                   
	uint32_t                cpu_type;
	uint32_t                cpu_subtype;
} KDP_PACKED kdp_hostinfo_t;

typedef struct {                                                
	kdp_hdr_t       hdr;
	kdp_hostinfo_t  hostinfo;
} KDP_PACKED kdp_hostinfo_reply_t;

  
              
   
typedef struct {                                                 
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_version_req_t;

#define KDP_FEATURE_BP  0x1                                   

typedef struct {                                               
	kdp_hdr_t       hdr;
	uint32_t        version;
	uint32_t        feature;
	uint32_t        pad0;
	uint32_t        pad1;
} KDP_PACKED kdp_version_reply_t;

#define VM_PROT_VOLATILE        ((vm_prot_t) 0x08)                         
#define VM_PROT_SPARSE          ((vm_prot_t) 0x10)                             

  
              
   
typedef struct {                                                 
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_regions_req_t;

typedef struct {
	uint32_t        address;
	uint32_t        nbytes;
	uint32_t        protection;                    
} KDP_PACKED kdp_region_t;

typedef struct {                                               
	kdp_hdr_t       hdr;
	uint32_t        nregions;
	kdp_region_t    regions[0];
} KDP_PACKED kdp_regions_reply_t;

  
               
   
typedef struct {                                                  
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_maxbytes_req_t;

typedef struct {                                                
	kdp_hdr_t       hdr;
	uint32_t        max_bytes;
} KDP_PACKED kdp_maxbytes_reply_t;

  
              
   
typedef struct {                                                 
	kdp_hdr_t       hdr;
	uint32_t        address;
	uint32_t        nbytes;
} KDP_PACKED kdp_readmem_req_t;

typedef struct {                                               
	kdp_hdr_t       hdr;
	kdp_error_t     error;
	char            data[0];
} KDP_PACKED kdp_readmem_reply_t;

  
                
   
typedef struct {                                                   
	kdp_hdr_t       hdr;
	uint64_t        address;
	uint32_t        nbytes;
} KDP_PACKED kdp_readmem64_req_t;

typedef struct {                                                 
	kdp_hdr_t       hdr;
	kdp_error_t     error;
	char            data[0];
} KDP_PACKED kdp_readmem64_reply_t;

  
                    
   
typedef struct {                                                       
	kdp_hdr_t       hdr;
	uint64_t        address;
	uint32_t        nbytes;
	uint16_t        lcpu;
} KDP_PACKED kdp_readphysmem64_req_t;

typedef struct {                                                     
	kdp_hdr_t       hdr;
	kdp_error_t     error;
	char            data[0];
} KDP_PACKED kdp_readphysmem64_reply_t;

  
               
   
typedef struct {                                                  
	kdp_hdr_t       hdr;
	uint32_t        address;
	uint32_t        nbytes;
	char            data[0];
} KDP_PACKED kdp_writemem_req_t;

typedef struct {                                                
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writemem_reply_t;

  
                 
   
typedef struct {                                                    
	kdp_hdr_t       hdr;
	uint64_t        address;
	uint32_t        nbytes;
	char            data[0];
} KDP_PACKED kdp_writemem64_req_t;

typedef struct {                                                  
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writemem64_reply_t;

  
                     
   
typedef struct {                                                        
	kdp_hdr_t       hdr;
	uint64_t        address;
	uint32_t        nbytes;
	uint16_t        lcpu;
	char            data[0];
} KDP_PACKED kdp_writephysmem64_req_t;

typedef struct {                                                      
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writephysmem64_reply_t;

  
                  
   
typedef struct {                                                     
	kdp_hdr_t       hdr;
	uint16_t        lcpu;
	uint16_t        address;
	uint16_t        nbytes;
	char            data[0];
} KDP_PACKED kdp_writeioport_req_t;

typedef struct {                                                   
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writeioport_reply_t;

  
                 
   
typedef struct {                                                    
	kdp_hdr_t       hdr;
	uint16_t        lcpu;
	uint16_t        address;
	uint16_t        nbytes;
} KDP_PACKED kdp_readioport_req_t;

typedef struct {                                                  
	kdp_hdr_t       hdr;
	kdp_error_t     error;
	char            data[0];
} KDP_PACKED kdp_readioport_reply_t;


  
                 
   
typedef struct {                                                    
	kdp_hdr_t       hdr;
	uint32_t        address;
	uint16_t        lcpu;
	char            data[0];
} KDP_PACKED kdp_writemsr64_req_t;

typedef struct {                                                  
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writemsr64_reply_t;

  
                
   
typedef struct {                                                   
	kdp_hdr_t       hdr;
	uint32_t        address;
	uint16_t        lcpu;
} KDP_PACKED kdp_readmsr64_req_t;

typedef struct {                                                 
	kdp_hdr_t       hdr;
	kdp_error_t     error;
	char            data[0];
} KDP_PACKED kdp_readmsr64_reply_t;


  
               
   
typedef struct {                                                  
	kdp_hdr_t       hdr;
	uint32_t        cpu;
	uint32_t        flavor;
} KDP_PACKED kdp_readregs_req_t;

typedef struct {                                                
	kdp_hdr_t       hdr;
	kdp_error_t     error;                                         
	char            data[0];
} KDP_PACKED kdp_readregs_reply_t;

  
                
   
typedef struct {                                                   
	kdp_hdr_t       hdr;
	uint32_t        cpu;
	uint32_t        flavor;
	char            data[0];
} KDP_PACKED kdp_writeregs_req_t;

typedef struct {                                                 
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_writeregs_reply_t;

  
           
   
typedef struct {                                              
	kdp_hdr_t       hdr;
	char            file_args[0];
} KDP_PACKED kdp_load_req_t;

typedef struct {                                            
	kdp_hdr_t       hdr;
	kdp_error_t     error;
} KDP_PACKED kdp_load_reply_t;

  
                
   
typedef struct {                                                   
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_imagepath_req_t;

typedef struct {                                                 
	kdp_hdr_t       hdr;
	char            path[0];
} KDP_PACKED kdp_imagepath_reply_t;

  
              
   
typedef struct {                                                 
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_suspend_req_t;

typedef struct {                                               
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_suspend_reply_t;

  
                 
   
typedef struct {                                                    
	kdp_hdr_t       hdr;
	uint32_t        cpu_mask;
} KDP_PACKED kdp_resumecpus_req_t;

typedef struct {                                                  
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_resumecpus_reply_t;

  
                                               
   

typedef struct {
	kdp_hdr_t hdr;
	uint32_t      address;
} KDP_PACKED kdp_breakpoint_req_t;

typedef struct {
	kdp_hdr_t hdr;
	kdp_error_t error;
} KDP_PACKED kdp_breakpoint_reply_t;

  
                                                   
   

typedef struct {
	kdp_hdr_t hdr;
	uint64_t        address;
} KDP_PACKED kdp_breakpoint64_req_t;

typedef struct {
	kdp_hdr_t hdr;
	kdp_error_t error;
} KDP_PACKED kdp_breakpoint64_reply_t;

  
                          
                                                                     
                                             
   
typedef struct {                                                   
	uint32_t        cpu;
	  
                                
                          
    
	uint32_t        exception;
	uint32_t        code;
	uint32_t        subcode;
} KDP_PACKED kdp_exc_info_t;

typedef struct {                                                        
	kdp_hdr_t       hdr;
	uint32_t        n_exc_info;
	kdp_exc_info_t  exc_info[0];
} KDP_PACKED kdp_exception_t;

typedef struct {                                                           
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_exception_ack_t;

  
                    
   
typedef struct {                                                       
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_kernelversion_req_t;

typedef struct {                                                     
	kdp_hdr_t       hdr;
	char            version[0];
} KDP_PACKED kdp_kernelversion_reply_t;


  
                             
   
typedef enum {
	KDP_FAULT = 0,                                               
	KDP_EXIT,                                 
	KDP_POWEROFF,                                
	KDP_REBOOT,                               
	KDP_COMMAND_MODE                                            
} kdp_termination_code_t;

typedef struct {                                                          
	kdp_hdr_t               hdr;
	uint32_t                term_code;                                  
	uint32_t                exit_code;
} KDP_PACKED kdp_termination_t;

typedef struct {
	kdp_hdr_t       hdr;
} KDP_PACKED kdp_termination_ack_t;

  
               
   
typedef struct {                                                  
	kdp_hdr_t       hdr;
	char            name[50];
	char            destip[16];
	char            routerip[16];
	uint32_t        port;
	kdp_dumpinfo_t  type;
} KDP_PACKED kdp_dumpinfo_req_t;

typedef struct {                                                
	kdp_hdr_t       hdr;
	char            name[50];
	char            destip[16];
	char            routerip[16];
	uint32_t        port;
	kdp_dumpinfo_t  type;
} KDP_PACKED kdp_dumpinfo_reply_t;


typedef union {
	kdp_hdr_t               hdr;
	kdp_connect_req_t       connect_req;
	kdp_connect_reply_t     connect_reply;
	kdp_disconnect_req_t    disconnect_req;
	kdp_disconnect_reply_t  disconnect_reply;
	kdp_hostinfo_req_t      hostinfo_req;
	kdp_hostinfo_reply_t    hostinfo_reply;
	kdp_version_req_t       version_req;
	kdp_version_reply_t     version_reply;
	kdp_maxbytes_req_t      maxbytes_req;
	kdp_maxbytes_reply_t    maxbytes_reply;
	kdp_readmem_req_t       readmem_req;
	kdp_readmem_reply_t     readmem_reply;
	kdp_readmem64_req_t     readmem64_req;
	kdp_readmem64_reply_t   readmem64_reply;
	kdp_readphysmem64_req_t readphysmem64_req;
	kdp_readphysmem64_reply_t       readphysmem64_reply;
	kdp_writemem_req_t      writemem_req;
	kdp_writemem_reply_t    writemem_reply;
	kdp_writemem64_req_t    writemem64_req;
	kdp_writemem64_reply_t  writemem64_reply;
	kdp_writephysmem64_req_t        writephysmem64_req;
	kdp_writephysmem64_reply_t      writephysmem64_reply;
	kdp_readregs_req_t      readregs_req;
	kdp_readregs_reply_t    readregs_reply;
	kdp_writeregs_req_t     writeregs_req;
	kdp_writeregs_reply_t   writeregs_reply;
	kdp_load_req_t          load_req;
	kdp_load_reply_t        load_reply;
	kdp_imagepath_req_t     imagepath_req;
	kdp_imagepath_reply_t   imagepath_reply;
	kdp_suspend_req_t       suspend_req;
	kdp_suspend_reply_t     suspend_reply;
	kdp_resumecpus_req_t    resumecpus_req;
	kdp_resumecpus_reply_t  resumecpus_reply;
	kdp_exception_t         exception;
	kdp_exception_ack_t     exception_ack;
	kdp_termination_t       termination;
	kdp_termination_ack_t   termination_ack;
	kdp_breakpoint_req_t    breakpoint_req;
	kdp_breakpoint_reply_t  breakpoint_reply;
	kdp_breakpoint64_req_t  breakpoint64_req;
	kdp_breakpoint64_reply_t        breakpoint64_reply;
	kdp_reattach_req_t      reattach_req;
	kdp_regions_req_t       regions_req;
	kdp_regions_reply_t     regions_reply;
	kdp_kernelversion_req_t kernelversion_req;
	kdp_kernelversion_reply_t       kernelversion_reply;
	kdp_readioport_req_t    readioport_req;
	kdp_readioport_reply_t  readioport_reply;
	kdp_writeioport_req_t   writeioport_req;
	kdp_writeioport_reply_t writeioport_reply;
	kdp_readmsr64_req_t     readmsr64_req;
	kdp_readmsr64_reply_t   readmsr64_reply;
	kdp_writemsr64_req_t    writemsr64_req;
	kdp_writemsr64_reply_t  writemsr64_reply;
	kdp_dumpinfo_req_t      dumpinfo_req;
	kdp_dumpinfo_reply_t    dumpinfo_reply;
} kdp_pkt_t;

#define MAX_KDP_PKT_SIZE        1200                         
#define MAX_KDP_DATA_SIZE       1024                                 

  
                                                   
                                                       
                   
   
#define KDP_MANUAL_PACKET_SIZE 128
struct kdp_manual_pkt {
	unsigned char       data[KDP_MANUAL_PACKET_SIZE];
	unsigned int        len;
	boolean_t           input;
} KDP_PACKED;

#ifdef KDP_PROXY_PACK_SUPPORT
#pragma pack()
#endif

#endif                    
