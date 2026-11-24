/*
 * Copyright (c) 2010 Apple Computer, Inc. All rights reserved.
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

#ifndef _SYS_PROCESS_POLICY_H
#define _SYS_PROCESS_POLICY_H

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdint.h>

#if defined(XNU_KERNEL_PRIVATE)

#if defined(XNU_TARGET_OS_OSX)
#define PROCESS_POLICY_OSX  1
#else                                 
#define PROCESS_POLICY_OSX  0
#endif                                 

#else                                  

#include <TargetConditionals.h>

#define PROCESS_POLICY_OSX !(TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
#endif                                  

__BEGIN_DECLS

                    
#define PROC_POLICY_SCOPE_PROCESS       1                                                          
#define PROC_POLICY_SCOPE_THREAD        2                                                           

                                          
#define PROC_POLICY_ACTION_APPLY        1                                   
#define PROC_POLICY_ACTION_RESTORE      2                                           
#define PROC_POLICY_ACTION_DENYINHERIT  3                                                           
#define PROC_POLICY_ACTION_DENYSELFSET  4                                                      
#define PROC_POLICY_ACTION_ENABLE       5                                          
#define PROC_POLICY_ACTION_DISABLE      6                                                                                               
                                        
#define PROC_POLICY_ACTION_SET          10                                     
#define PROC_POLICY_ACTION_GET          11                                     
#define PROC_POLICY_ACTION_ADD          12                                  
#define PROC_POLICY_ACTION_REMOVE       13                                     
#define PROC_POLICY_ACTION_HOLD         14                                              
#define PROC_POLICY_ACTION_DROP         15                                              

              
#define PROC_POLICY NONE		0
#define PROC_POLICY_BACKGROUND          1                                     
#define PROC_POLICY_HARDWARE_ACCESS     2                                       
#define PROC_POLICY_RESOURCE_STARVATION 3                                            
#define PROC_POLICY_RESOURCE_USAGE      4                                             
#if !PROCESS_POLICY_OSX
#define PROC_POLICY_APP_LIFECYCLE       5                                      
#else                          
#define PROC_POLICY_RESERVED            5                                             
#endif                          
#define PROC_POLICY_APPTYPE             6                                             
#define PROC_POLICY_BOOST               7                                  
#define PROC_POLICY_NO_SMT              8                                                  
#define PROC_POLICY_TECS                9                                            

                                        
#define PROC_POLICY_BG_NONE             0                 
#define PROC_POLICY_BG_LOWCPUPRI        1                             
#define PROC_POLICY_BG_DISKTHROTTLE     2                                    
#define PROC_POLICY_BG_NETTHROTTLE      4                                       
#define PROC_POLICY_BG_GPUDENY          8                             
#if !PROCESS_POLICY_OSX
#define PROC_POLICY_BG_ALL            0x0F
#else                          
#define PROC_POLICY_BG_ALL            0x07
#endif                          
#define PROC_POLICY_BG_DEFAULT          PROC_POLICY_BG_ALL

                               
#define PROC_POLICY_HWACCESS_NONE       0
#define PROC_POLICY_HWACCESS_DISK       1                        
#define PROC_POLICY_HWACCESS_GPU        2                       
#define PROC_POLICY_HWACCESS_NETWORK    3                           
#define PROC_POLICY_HWACCESS_CPU        4                       

                                                                                             
#define PROC_POLICY_DISKACC_NONE        0
#define PROC_POLICY_DISKACC_NORMAL      1                                      
#define PROC_POLICY_DISKACC_FULLACCESS  1                                      
#define PROC_POLICY_DISKACC_PASSIVE     2                                      
#define PROC_POLICY_DISKACC_THROTTLE    3                                  
#define PROC_POLICY_DISKACC_DEFAULT     PROC_POLICY_DISKACC_FULLACCESS

                                              
#define PROC_POLICY_GPUACC_NONE 0
#define PROC_POLICY_GPUACC_FULLACCESS   0                                       
#define PROC_POLICY_GPUACC_DENYACCESS   1                                       
#define PROC_POLICY_GPUACC_DEFAULT      PROC_POLICY_GPUACC_FULLACCESS                                  

                                                   
#define PROC_POLICY_NETACC_NONE 0
#define PROC_POLICY_NETACC_FULLACCESS   0                                           
#define PROC_POLICY_NETACC_THROTTLE     1                                       
#define PROC_POLICY_NETACC_DEFAULT      PROC_POLICY_NETACC_FULLACCESS                                  

                                                   
#define PROC_POLICY_CPUACC_NONE         0
#define PROC_POLICY_CPUACC_FULLACCESS   0                                         
#define PROC_POLICY_CPUACC_ONE          1                                             
#define PROC_POLICY_CPUACC_LLCACHE      2                                                
#define PROC_POLICY_CPUACC_DEFAULT      PROC_POLICY_CPUACC_FULLACCESS                                     


                                                                              

                                          
#define PROC_POLICY_RS_NONE             0
#define PROC_POLICY_RS_VIRTUALMEM       1                                      

                                     
#define PROC_POLICY_RUSAGE_NONE         0
#define PROC_POLICY_RUSAGE_WIREDMEM     1                                
#define PROC_POLICY_RUSAGE_VIRTMEM      2                                 
#define PROC_POLICY_RUSAGE_CPU          3                                
#define PROC_POLICY_RUSAGE_DISK         4                                 
#define PROC_POLICY_RUSAGE_NETWORK      5                                    
#define PROC_POLICY_RUSAGE_POWER        6                                                

                                                                                                          
#define PROC_POLICY_RSRCACT_NONE        0
#define PROC_POLICY_RSRCACT_THROTTLE    1                                           
#define PROC_POLICY_RSRCACT_SUSPEND     2                                          
#define PROC_POLICY_RSRCACT_TERMINATE   3                                       
#define PROC_POLICY_RSRCACT_NOTIFY_KQ   4                                     
#define PROC_POLICY_RSRCACT_NOTIFY_EXC  5                           

#define PROC_POLICY_CPUMON_DISABLE      0xFF                                   
#define PROC_POLICY_CPUMON_DEFAULTS     0xFE                                              

                                            
#define PROC_POLICY_IMP_IMPORTANT       1                                  
#define PROC_POLICY_IMP_STANDARD        2                                 
#define PROC_POLICY_IMP_DONATION        3                                                

typedef struct proc_policy_attribute {
	uint32_t        ppattr_attribute;                                                       
	uint32_t        ppattr_resv;                      
	uint64_t        ppattr_value1;                                          
	uint64_t        ppattr_value2;                                          
	uint64_t        ppattr_value3;                                          
	uint64_t        ppattr_resv1[4];                                 
} proc_policy_attribute_t;


typedef struct proc_policy_cpuusage_attr {
	uint32_t        ppattr_cpu_attr;                                                     
	uint32_t        ppattr_cpu_percentage;                                   
	uint64_t        ppattr_cpu_attr_interval;                                  
	uint64_t        ppattr_cpu_attr_deadline;                                  
} proc_policy_cpuusage_attr_t;

#if !PROCESS_POLICY_OSX
                                               
#define PROC_POLICY_APPLIFE_NONE        0                           
#define PROC_POLICY_APPLIFE_STATE       1                                                     
#define PROC_POLICY_APPLIFE_DEVSTATUS   2                                                            
#define PROC_POLICY_APPLIFE_PIDBIND     3                                                                   
#endif                          

                                          
#define PROC_POLICY_APPTYPE_NONE        0                           
#define PROC_POLICY_APPTYPE_MODIFY      1                                                     
#if !PROCESS_POLICY_OSX
#define PROC_POLICY_APPTYPE_THREADTHR   2                                                            
#endif                          

                                               
#define PROC_POLICY_OSX_APPTYPE_TAL             1                             

#define PROC_POLICY_OSX_APPTYPE_DASHCLIENT      2                                          
#define PROC_POLICY_IOS_DONATEIMP               6                                              
#define PROC_POLICY_IOS_HOLDIMP                 7                                                   
#define PROC_POLICY_IOS_DROPIMP                 8                                                   

#ifndef KERNEL
int process_policy(int scope, int action, int policy, int policy_subtype, proc_policy_attribute_t * attrp, pid_t target_pid, uint64_t target_threadid);
#endif             


__END_DECLS

#endif                           
