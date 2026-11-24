/*
 * Copyright (c) 2004-2005, 2011 Apple Computer, Inc. All rights reserved.
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
/*
 * Copyright (c) 1993, David Greenman
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
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
 */
  
                                                                      
                                                                          
                                                                        
               
   
#ifndef _SYS_IMGACT_H_
#define _SYS_IMGACT_H_

#define IMG_SHSIZE      512                                              

struct label;
struct proc;
struct nameidata;

struct image_params {
	user_addr_t     ip_user_fname;                        
	user_addr_t     ip_user_argv;                         
	user_addr_t     ip_user_envv;                         
	int             ip_seg;                                            
	struct vnode    *ip_vp;                           
	struct vnode_attr       *ip_vattr;                               
	struct vnode_attr       *ip_origvattr;                                  
	cpu_type_t      ip_origcputype;                                         
	cpu_subtype_t   ip_origcpusubtype;                                      
	char            *ip_vdata;                                              
	int             ip_flags;                                
	int             ip_argc;                                    
	int             ip_envc;                                       
	int             ip_applec;                                      

	char            *ip_startargv;                                         
	char            *ip_endargv;                                   
	char            *ip_endenvv;                                     

	char            *ip_strings;                                          
	char            *ip_strendp;                                     

	char            *ip_subsystem_root_path;                                             

	int             ip_argspace;                                                     
	int             ip_strspace;                                              

	user_size_t     ip_arch_offset;                                      
	user_size_t     ip_arch_size;                                        
	char            ip_interp_buffer[IMG_SHSIZE];                                 
	int             ip_interp_sugid_fd;                                      

	                                                                    
	struct vfs_context      *ip_vfs_context;                         
	struct nameidata *ip_ndp;                                      
	thread_t        ip_new_thread;                                      

	struct label    *ip_execlabelp;                                      
	struct label    *ip_scriptlabelp;                                
	struct vnode    *ip_scriptvp;                       
	unsigned int    ip_csflags;                                     
	int             ip_mac_return;                                                  
	void            *ip_px_sa;
	void            *ip_px_sfa;
	void            *ip_px_spa;
	void            *ip_px_smpx;                                           
	void            *ip_px_persona;                           
	void            *ip_px_pcred_info;                           
	void            *ip_cs_error;                                         
	char            *ip_inherited_shared_region_id;                                               

	uint64_t ip_dyld_fsid;
	uint64_t ip_dyld_fsobjid;
	uint64_t ip_inherited_jop_pid;
	unsigned int    ip_simulator_binary;                                

	ipc_port_t      ip_sc_port;                             
};

  
              
   
#define IMGPF_NONE                              0x00000000                    
#define IMGPF_INTERPRET                 0x00000001                               
#define IMGPF_RESERVED                  0x00000002
#define IMGPF_WAS_64BIT_ADDR    0x00000004                                           
#define IMGPF_IS_64BIT_ADDR             0x00000008                                         
#define IMGPF_SPAWN                             0x00000010                                   
#define IMGPF_DISABLE_ASLR              0x00000020                        
#define IMGPF_ALLOW_DATA_EXEC   0x00000040                                            
#define IMGPF_VFORK_EXEC                0x00000080                                  
#define IMGPF_EXEC                              0x00000100                
#define IMGPF_HIGH_BITS_ASLR    0x00000200                                             
#define IMGPF_IS_64BIT_DATA             0x00000400                                          
#define IMGPF_DRIVER             0x00000800                                                
#define IMGPF_RESLIDE           0x000001000                                   
#define IMGPF_PLUGIN_HOST_DISABLE_A_KEYS  0x000002000                                                         
#define IMGPF_NOJOP             0x80000000


  
                         
   
#define IMGPF_SB_DEFAULT         0                                                                             
#define IMGPF_SB_TRUE            1                                                 
#define IMGPF_SB_FALSE           2                                                     


#endif                    
