/*
 * Copyright (c) 2010-2017 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 2006 nCircle Network Security, Inc.
 * All rights reserved.
 *
 * This software was developed by Robert N. M. Watson for the TrustedBSD
 * Project under contract to nCircle Network Security, Inc.
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
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR, NCIRCLE NETWORK SECURITY,
 * INC., OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/priv.h,v 1.38.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $
 */

  
                                       
   
#ifndef _SYS_PRIV_H_
#define _SYS_PRIV_H_

  
                                                      
  
                                                                          
                                                                        
                                                                      
   

  
                                                                  
                                                                     
                                                               
                                                                 
                                            
   
#define PRIV_ADJTIME                    1000                              
#define PRIV_PROC_UUID_POLICY           1001                                           
#define PRIV_GLOBAL_PROC_INFO           1002                                                              
#define PRIV_SYSTEM_OVERRIDE            1003                                                                                                   
#define PRIV_HW_DEBUG_DATA              1004                                                        
#define PRIV_SELECTIVE_FORCED_IDLE      1005                                                                     
#define PRIV_PROC_TRACE_INSPECT         1006                                                                   
#define PRIV_DARKBOOT                   1007                                      
#define PRIV_WORK_INTERVAL              1008                                               
#define PRIV_SMB_TIMEMACHINE_CONTROL    1009                                                         
#define PRIV_AUDIO_LATENCY              1010                                                               
#define PRIV_KTRACE_BACKGROUND          1011                                          
#define PRIV_SETPRIORITY_DARWIN_ROLE    1012                                             
#define PRIV_PACKAGE_EXTENSIONS         1013                                                                     
#define PRIV_TRIM_ACTIVE_FILE           1014                                                            
#define PRIV_PROC_CPUMON_OVERRIDE       1015                                                                          
#define PRIV_ENDPOINTSECURITY_CLIENT    1016                                                   
#define PRIV_AUDIT_SESSION_PORT         1017                                                               

  
                             
   
#define PRIV_VM_PRESSURE        6000                            
#define PRIV_VM_JETSAM          6001                                      
#define PRIV_VM_FOOTPRINT_LIMIT 6002                                          

  
                            
   
#define PRIV_NET_PRIVILEGED_TRAFFIC_CLASS       10000                                         
#define PRIV_NET_PRIVILEGED_SOCKET_DELEGATE     10001                                 
#define PRIV_NET_INTERFACE_CONTROL              10002                                        
#define PRIV_NET_PRIVILEGED_NETWORK_STATISTICS  10003                              
#define PRIV_NET_PRIVILEGED_NECP_POLICIES       10004                                                        
#define PRIV_NET_RESTRICTED_AWDL                10005                                       
#define PRIV_NET_PRIVILEGED_NECP_MATCH          10006                                                         
#define PRIV_NET_QOSMARKING_POLICY_OVERRIDE     10007                                                         
#define PRIV_NET_RESTRICTED_INTCOPROC           10008                                                           

#define PRIV_NET_PRIVILEGED_MULTIPATH           10009                        
#define PRIV_NET_RESTRICTED_MULTIPATH_EXTENDED  10010                                                     
#define PRIV_NET_RESTRICTED_ROUTE_NC_READ       10011                                                     

#define PRIV_NET_PRIVILEGED_CLIENT_ACCESS       10012                                                               
#define PRIV_NET_PRIVILEGED_SERVER_ACCESS       10013                                                               

#define PRIV_NET_VALIDATED_RESOLVER             10014                                                              

#define PRIV_NET_CUSTOM_PROTOCOL                10015                                              
#define PRIV_NET_PRIVILEGED_NECP_DROP_ALL_BYPASS 10016                                         
#define PRIV_NET_PRIVILEGED_IPSEC_WAKE_PACKET   10017                                           

  
                            
   
#define PRIV_NETINET_RESERVEDPORT       11000                              
#define PRIV_NETINET_TCP_KA_OFFLOAD     11001                                              


  
                 
   
#define PRIV_VFS_OPEN_BY_ID             14000                                    
#define PRIV_VFS_MOVE_DATA_EXTENTS      14001                                      
#define PRIV_VFS_SNAPSHOT               14002                                                
#define PRIV_VFS_SNAPSHOT_REVERT        14003                                                          
#define PRIV_VFS_DATALESS_RESOLVER      14004                                                     
#define PRIV_VFS_DATALESS_MANIPULATION  14005                                                                                    
#define PRIV_VFS_SETSIZE                14006                                                    

#define PRIV_APFS_EMBED_DRIVER          14100                                                              
#define PRIV_APFS_DEBUG                 14101                                                                    
#define PRIV_APFS_FUSION_DEBUG          14101                                                       
                                                                                                                              
                                                                                                                       
#define PRIV_APFS_FUSION_ALLOW_PIN_FASTPROMOTE  14102                                                                               
                                              
#define PRIV_APFS_SET_FREE_SPACE_CHANGE_THRESHOLD       14104                                                                   
#define PRIV_APFS_SET_FIRMLINK       14105                                                

#ifdef KERNEL
  
                                                                          
   
#include <sys/cdefs.h>
#include <sys/kauth.h>

  
                            
   
#define PRIVCHECK_DEFAULT_UNPRIVILEGED_FLAG (1)                                            

__BEGIN_DECLS
int     priv_check_cred(kauth_cred_t cred, int priv, int flags);
__END_DECLS
#endif

#endif                    
