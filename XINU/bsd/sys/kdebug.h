/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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

#ifndef BSD_SYS_KDEBUG_H
#define BSD_SYS_KDEBUG_H

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#ifdef __APPLE_API_UNSTABLE

  
                                                                              
                                                      
  
                                                                               
              
  
                                                                        
                                                                        
                                                                        
                                                                        
                                      
                              
                                                                        
                                          
                                                                        
                                          
  
                                                                            
                                                                            
                                                                         
                               
   

#define KDBG_CLASS_MASK   (0xff000000)
#define KDBG_CLASS_OFFSET (24)
#define KDBG_CLASS_MAX    (0xff)

#define KDBG_SUBCLASS_MASK   (0x00ff0000)
#define KDBG_SUBCLASS_OFFSET (16)
#define KDBG_SUBCLASS_MAX    (0xff)

                             
#define KDBG_CSC_MASK   (0xffff0000)
#define KDBG_CSC_OFFSET (KDBG_SUBCLASS_OFFSET)
#define KDBG_CSC_MAX    (0xffff)

#define KDBG_CODE_MASK   (0x0000fffc)
#define KDBG_CODE_OFFSET (2)
#define KDBG_CODE_MAX    (0x3fff)

#define KDBG_EVENTID_MASK (0xfffffffc)
#define KDBG_FUNC_MASK    (0x00000003)

                                                                     
#define KDBG_EVENTID(Class, SubClass, Code)                \
	(((unsigned)((Class)    &   0xff) << KDBG_CLASS_OFFSET)    | \
	 ((unsigned)((SubClass) &   0xff) << KDBG_SUBCLASS_OFFSET) | \
	 ((unsigned)((Code)     & 0x3fff) << KDBG_CODE_OFFSET))
                                                   
#define KDBG_CODE(Class, SubClass, Code) \
	KDBG_EVENTID(Class, SubClass, Code)

                                       
#define KDBG_EXTRACT_CLASS(Debugid) \
	((uint8_t)(((Debugid) & KDBG_CLASS_MASK) >> KDBG_CLASS_OFFSET))
#define KDBG_EXTRACT_SUBCLASS(Debugid) \
	((uint8_t)(((Debugid) & KDBG_SUBCLASS_MASK) >> KDBG_SUBCLASS_OFFSET))
#define KDBG_EXTRACT_CSC(Debugid) \
	((uint16_t)(((Debugid) & KDBG_CSC_MASK) >> KDBG_CSC_OFFSET))
#define KDBG_EXTRACT_CODE(Debugid) \
	((uint16_t)(((Debugid) & KDBG_CODE_MASK) >> KDBG_CODE_OFFSET))
#define KDBG_CLASS_ENCODE(Class, SubClass) KDBG_EVENTID(Class, SubClass, 0)
#define KDBG_CLASS_DECODE(Debugid) (Debugid & KDBG_CSC_MASK)

                          
#define DBG_FUNC_START 1U
#define DBG_FUNC_END   2U
#define DBG_FUNC_NONE  0U

                               

#define DBG_MACH        1
#define DBG_NETWORK     2
#define DBG_FSYSTEM     3
#define DBG_BSD         4
#define DBG_IOKIT       5
#define DBG_DRIVERS     6
#define DBG_TRACE       7
#define DBG_DLIL        8
#define DBG_PTHREAD     9
#define DBG_CORESTORAGE 10
#define DBG_CG          11
#define DBG_MONOTONIC   12
#define DBG_MISC        20
#define DBG_SECURITY    30
#define DBG_DYLD        31
#define DBG_QT          32
#define DBG_APPS        33
#define DBG_LAUNCHD     34
#define DBG_SILICON     35
#define DBG_PERF        37
#define DBG_IMPORTANCE  38
#define DBG_BANK        40
#define DBG_XPC         41
#define DBG_ATM         42
#define DBG_ARIADNE     43
#define DBG_DAEMON      44
#define DBG_ENERGYTRACE 45
#define DBG_DISPATCH    46
#define DBG_IMG         49
#define DBG_UMALLOC     51
#define DBG_TURNSTILE   53

#define DBG_MIG         255

                                                                
#define DBG_MACH_EXCP_KTRAP_x86 0x02                          
#define DBG_MACH_EXCP_DFLT      0x03                      
#define DBG_MACH_EXCP_SYNC_ARM  0x03                                      
#define DBG_MACH_EXCP_IFLT      0x04                      
#define DBG_MACH_EXCP_SERR_ARM  0x04                                         
#define DBG_MACH_EXCP_INTR      0x05                 
#define DBG_MACH_EXCP_ALNG      0x06                          
#define DBG_MACH_EXCP_UTRAP_x86 0x07                        
#define DBG_MACH_EXCP_FP        0x08                 
#define DBG_MACH_EXCP_DECI      0x09                            
#define DBG_MACH_CHUD           0x0A                      
#define DBG_MACH_SIGNPOST       0x0A                       
#define DBG_MACH_EXCP_SC        0x0C                   
#define DBG_MACH_EXCP_TRACE     0x0D                      
#define DBG_MACH_EXCP_EMUL      0x0E                           
#define DBG_MACH_IHDLR          0x10                         
#define DBG_MACH_IPC            0x20                         
#define DBG_MACH_RESOURCE       0x25                          
#define DBG_MACH_VM             0x30                     
#define DBG_MACH_LEAKS          0x31                 
#define DBG_MACH_WORKINGSET     0x32                                                         
#define DBG_MACH_SCHED          0x40                
#define DBG_MACH_MSGID_INVALID  0x50                         
#define DBG_MACH_LOCKS          0x60                    
#define DBG_MACH_PMAP           0x70           
#define DBG_MACH_CLOCK          0x80            
#define DBG_MACH_MP             0x90                 
#define DBG_MACH_VM_PRESSURE    0xA0                             
#define DBG_MACH_STACKSHOT      0xA1                                         
#define DBG_MACH_SFI            0xA2                                  
#define DBG_MACH_ENERGY_PERF    0xA3                                        
#define DBG_MACH_SYSDIAGNOSE    0xA4                  
#define DBG_MACH_ZALLOC         0xA5                     
#define DBG_MACH_THREAD_GROUP   0xA6                    
#define DBG_MACH_COALITION      0xA7                 
#define DBG_MACH_SHAREDREGION   0xA8                    
#define DBG_MACH_SCHED_CLUTCH   0xA9                       
#define DBG_MACH_IO             0xAA          
#define DBG_MACH_WORKGROUP      0xAB                          
#define DBG_MACH_HV             0xAC                           

                           
#define DBC_MACH_IO_MMIO_READ           0x1
#define DBC_MACH_IO_MMIO_WRITE          0x2
#define DBC_MACH_IO_PHYS_READ           0x3
#define DBC_MACH_IO_PHYS_WRITE          0x4
#define DBC_MACH_IO_PORTIO_READ         0x5
#define DBC_MACH_IO_PORTIO_WRITE        0x6

                                                
#define DBG_INTR_TYPE_UNKNOWN   0x0                                    
#define DBG_INTR_TYPE_IPI       0x1                                   
#define DBG_INTR_TYPE_TIMER     0x2                          
#define DBG_INTR_TYPE_OTHER     0x3                                             
#define DBG_INTR_TYPE_PMI       0x4                                        

                                          
#define MACH_SCHED              0x0                    
#define MACH_STACK_ATTACH       0x1                         
#define MACH_STACK_HANDOFF      0x2                          
#define MACH_CALL_CONT          0x3                              
#define MACH_CALLOUT            0x4                   
#define MACH_STACK_DETACH       0x5
#define MACH_MAKE_RUNNABLE      0x6                               
#define MACH_PROMOTE            0x7                                                               
#define MACH_DEMOTE             0x8                                                         
#define MACH_IDLE               0x9                           
#define MACH_STACK_DEPTH        0xa                                
#define MACH_MOVED              0xb                                                   
#define MACH_PSET_LOAD_AVERAGE  0xc
#define MACH_AMP_DEBUG          0xd
#define MACH_FAILSAFE           0xe                                        
#define MACH_BLOCK              0xf                       
#define MACH_WAIT               0x10                               
#define MACH_GET_URGENCY        0x14                                     
#define MACH_URGENCY            0x15                                          
                                               
                                    
#define MACH_REDISPATCH         0x16                                           
#define MACH_REMOTE_AST         0x17                                               
#define MACH_SCHED_CHOOSE_PROCESSOR     0x18                                    
#define MACH_DEEP_IDLE          0x19                                       
                                                                        
#define MACH_CPU_THROTTLE_DISABLE       0x1b                                     
#define MACH_RW_PROMOTE            0x1c                                        
#define MACH_RW_DEMOTE             0x1d                                      
#define MACH_SCHED_MAINTENANCE     0x1f                                  
#define MACH_DISPATCH              0x20                               
#define MACH_QUANTUM_HANDOFF       0x21                               
#define MACH_MULTIQ_DEQUEUE        0x22                               
#define MACH_SCHED_THREAD_SWITCH   0x23                                                     
#define MACH_SCHED_SMT_BALANCE     0x24                              
#define MACH_REMOTE_DEFERRED_AST   0x25                                                    
#define MACH_REMOTE_CANCEL_AST     0x26                                                 
#define MACH_SCHED_CHANGE_PRIORITY 0x27                                    
#define MACH_SCHED_UPDATE_REC_CORES     0x28                                                 
#define MACH_STACK_WAIT            0x29                                                                       
#define MACH_THREAD_BIND           0x2a                                                   
#define MACH_WAITQ_PROMOTE         0x2b                                     
#define MACH_WAITQ_DEMOTE          0x2c                                      
#define MACH_SCHED_LOAD            0x2d                  
#define MACH_REC_CORES_FAILSAFE    0x2e                                               
#define MACH_SCHED_QUANTUM_EXPIRED 0x2f                             
#define MACH_EXEC_PROMOTE          0x30                                    
#define MACH_EXEC_DEMOTE           0x31                                     
#define MACH_AMP_SIGNAL_SPILL      0x32                                     
#define MACH_AMP_STEAL             0x33                                   
#define MACH_SCHED_LOAD_EFFECTIVE  0x34                               
                                                                                                  
                                                                                                    
                                                                                                          
#define MACH_QUIESCENT_COUNTER     0x38                             
#define MACH_TURNSTILE_USER_CHANGE 0x39                                                
#define MACH_AMP_RECOMMENDATION_CHANGE 0x3a                                         
#define MACH_AMP_PERFCTL_POLICY_CHANGE 0x3b                                                    
#define MACH_TURNSTILE_KERNEL_CHANGE 0x40                                                 
#define MACH_SCHED_WI_AUTO_JOIN      0x41                                     
#define MACH_SCHED_WI_DEFERRED_FINISH 0x42                                                                      
#define MACH_SET_RT_DEADLINE       0x43                                    
#define MACH_CANCEL_RT_DEADLINE    0x44                                       
#define MACH_PSET_AVG_EXEC_TIME    0x50

                                                             
#define MACH_SCHED_CLUTCH_ROOT_BUCKET_STATE     0x0               
#define MACH_SCHED_CLUTCH_TG_BUCKET_STATE       0x1               
#define MACH_SCHED_CLUTCH_THREAD_SELECT         0x2                                                   
#define MACH_SCHED_CLUTCH_THREAD_STATE          0x3               
#define MACH_SCHED_CLUTCH_TG_BUCKET_PRI         0x4                                          
                                
#define MACH_SCHED_EDGE_CLUSTER_OVERLOAD        0x5                                                                        
#define MACH_SCHED_EDGE_STEAL                   0x6                                             
#define MACH_SCHED_EDGE_REBAL_RUNNABLE          0x7                                                      
#define MACH_SCHED_EDGE_REBAL_RUNNING           0x8                                                     
#define MACH_SCHED_EDGE_SHOULD_YIELD            0x9                                      
#define MACH_SCHED_CLUTCH_THR_COUNT             0xa                                              
#define MACH_SCHED_EDGE_LOAD_AVG                0xb                               

                                                                 
#define WORKGROUP_INTERVAL_CREATE               0x0                             
#define WORKGROUP_INTERVAL_DESTROY              0x1                                
#define WORKGROUP_INTERVAL_CHANGE               0x2                                  
#define WORKGROUP_INTERVAL_START                0x3                               
#define WORKGROUP_INTERVAL_UPDATE               0x4                                
#define WORKGROUP_INTERVAL_FINISH               0x5                                

                                      
#define MACH_MULTIQ_BOUND     1
#define MACH_MULTIQ_GROUP     2
#define MACH_MULTIQ_GLOBAL    3

                                          
#define DBG_ZERO_FILL_FAULT           1
#define DBG_PAGEIN_FAULT              2
#define DBG_COW_FAULT                 3
#define DBG_CACHE_HIT_FAULT           4
#define DBG_NZF_PAGE_FAULT            5
#define DBG_GUARD_FAULT               6
#define DBG_PAGEINV_FAULT             7
#define DBG_PAGEIND_FAULT             8
#define DBG_COMPRESSOR_FAULT          9
#define DBG_COMPRESSOR_SWAPIN_FAULT  10
#define DBG_COR_FAULT                11

                                  
#define MACH_TASK_SUSPEND                       0x0                           
#define MACH_TASK_RESUME                        0x1                         
#define MACH_THREAD_SET_VOUCHER                 0x2
#define MACH_IPC_MSG_SEND                       0x3                                       
#define MACH_IPC_MSG_RECV                       0x4                           
#define MACH_IPC_MSG_RECV_VOUCHER_REFUSED       0x5                                            
#define MACH_IPC_KMSG_FREE                      0x6                                   
#define MACH_IPC_VOUCHER_CREATE                 0x7                                                    
#define MACH_IPC_VOUCHER_CREATE_ATTR_DATA       0x8                                              
#define MACH_IPC_VOUCHER_DESTROY                0x9                                                        
#define MACH_IPC_KMSG_INFO                      0xa                                       
#define MACH_IPC_KMSG_LINK                      0xb                                                               
#define MACH_IPC_PORT_ENTRY_MODIFY              0xc                                                               
#define MACH_IPC_DESTROY_GUARDED_DESC           0xd                                                 

                                                     
#define MACH_THREAD_GROUP_NEW           0x0
#define MACH_THREAD_GROUP_FREE          0x1
#define MACH_THREAD_GROUP_SET           0x2
#define MACH_THREAD_GROUP_NAME          0x3
#define MACH_THREAD_GROUP_NAME_FREE     0x4
#define MACH_THREAD_GROUP_FLAGS         0x5
#define MACH_THREAD_GROUP_BLOCK         0x6

                                               
#define MACH_COALITION_NEW                      0x0
#define MACH_COALITION_FREE                     0x1
#define MACH_COALITION_ADOPT                    0x2
#define MACH_COALITION_REMOVE                   0x3
#define MACH_COALITION_THREAD_GROUP_SET         0x4

                                    
#define PMAP__CREATE            0x0
#define PMAP__DESTROY           0x1
#define PMAP__PROTECT           0x2
#define PMAP__PAGE_PROTECT      0x3
#define PMAP__ENTER             0x4
#define PMAP__REMOVE            0x5
#define PMAP__NEST              0x6
#define PMAP__UNNEST            0x7
#define PMAP__FLUSH_TLBS        0x8
#define PMAP__UPDATE_INTERRUPT  0x9
#define PMAP__ATTRIBUTE_CLEAR   0xa
#define PMAP__REUSABLE          0xb                                    
#define PMAP__QUERY_RESIDENT    0xc
#define PMAP__FLUSH_KERN_TLBS   0xd
#define PMAP__FLUSH_DELAYED_TLBS        0xe
#define PMAP__FLUSH_TLBS_TO     0xf
#define PMAP__FLUSH_EPT         0x10
#define PMAP__FAST_FAULT        0x11
#define PMAP__SWITCH            0x12
#define PMAP__TTE               0x13
#define PMAP__SWITCH_USER_TTB   0x14
#define PMAP__UPDATE_CACHING    0x15
#define PMAP__ATTRIBUTE_CLEAR_RANGE 0x16
#define PMAP__CLEAR_USER_TTB    0x17
#define PMAP__IOMMU_INIT        0x18
#define PMAP__IOMMU_IOVMALLOC   0x19
#define PMAP__IOMMU_IOVMFREE    0x1a
#define PMAP__IOMMU_MAP         0x1b
#define PMAP__IOMMU_UNMAP       0x1c
#define PMAP__IOMMU_IOCTL       0x1d
#define PMAP__IOMMU_GRANT_PAGE  0x1e

                                      
#define MACH_EPOCH_CHANGE       0x0                            
#define MACH_BRIDGE_RCV_TS      0x1                                                        
#define MACH_BRIDGE_REMOTE_TIME 0x2                                     
#define MACH_BRIDGE_RESET_TS    0x3                                                
#define MACH_BRIDGE_TS_PARAMS   0x4                                                    
#define MACH_BRIDGE_SKIP_TS     0x5                         
#define MACH_BRIDGE_TS_MISMATCH 0x6                                                                   
#define MACH_BRIDGE_OBSV_RATE   0x7                                      

                                                             
#define MICROSTACKSHOT_RECORD   0x0
#define MICROSTACKSHOT_GATHER   0x1

                                                  
#define SYSDIAGNOSE_NOTIFY_USER 0x0
#define SYSDIAGNOSE_FULL        0x1
#define SYSDIAGNOSE_STACKSHOT   0x2
#define SYSDIAGNOSE_TAILSPIN    0x3

                                                    
#define SFI_SET_WINDOW                  0x0
#define SFI_CANCEL_WINDOW               0x1
#define SFI_SET_CLASS_OFFTIME           0x2
#define SFI_CANCEL_CLASS_OFFTIME        0x3
#define SFI_THREAD_DEFER                0x4
#define SFI_OFF_TIMER                   0x5
#define SFI_ON_TIMER                    0x6
#define SFI_WAIT_CANCELED               0x7
#define SFI_PID_SET_MANAGED             0x8
#define SFI_PID_CLEAR_MANAGED           0x9
#define SFI_GLOBAL_DEFER                0xa

                                                
#define ZALLOC_ZCRAM                    0x0

                                                            
                                                  
#define RMON_ENABLE_CPUUSAGE_MONITOR    0x001
#define RMON_CPUUSAGE_VIOLATED          0x002
#define RMON_CPUUSAGE_SUSPENDED         0x003
#define RMON_CPUUSAGE_VIOLATED_K32A     0x004
#define RMON_CPUUSAGE_VIOLATED_K32B     0x005
#define RMON_CPUUSAGE_RESUMED           0x006
#define RMON_DISABLE_CPUUSAGE_MONITOR   0x00f

#define RMON_ENABLE_CPUWAKES_MONITOR    0x011
#define RMON_CPUWAKES_VIOLATED          0x012
#define RMON_CPUWAKES_VIOLATED_K32A     0x014
#define RMON_CPUWAKES_VIOLATED_K32B     0x015
#define RMON_DISABLE_CPUWAKES_MONITOR   0x01f

#define RMON_ENABLE_IO_MONITOR          0x021
#define RMON_LOGWRITES_VIOLATED         0x022
#define RMON_PHYSWRITES_VIOLATED        0x023
#define RMON_LOGWRITES_VIOLATED_K32A    0x024
#define RMON_LOGWRITES_VIOLATED_K32B    0x025
#define RMON_DISABLE_IO_MONITOR         0x02f

                                        
#define HV_GUEST_ENTER                  0x000
#define HV_GUEST_ERROR                  0x001

                                                                      
#define DBG_NETIP       1                              
#define DBG_NETARP      2                                        
#define DBG_NETUDP      3                                   
#define DBG_NETTCP      4                                          
#define DBG_NETICMP     5                                              
#define DBG_NETIGMP     6                                               
#define DBG_NETRIP      7                                         
#define DBG_NETOSPF     8                                     
#define DBG_NETISIS     9                                                       
#define DBG_NETSNMP     10                                              
#define DBG_NETSOCK     11                        

                    
#define DBG_NETAARP     100                    
#define DBG_NETDDP      101                                     
#define DBG_NETNBP      102                                
#define DBG_NETZIP      103                                    
#define DBG_NETADSP     104                                
#define DBG_NETATP      105                                     
#define DBG_NETASP      106                                 
#define DBG_NETAFP      107                                
#define DBG_NETRTMP     108                                             
#define DBG_NETAURP     109                                        

#define DBG_NETIPSEC    128                          
#define DBG_NETVMNET    129                

                                                                  
#define DBG_IOINTC                      0                                 
#define DBG_IOWORKLOOP                  1                                
#define DBG_IOINTES                     2                                   
#define DBG_IOCLKES                     3                               
#define DBG_IOCMDQ                      4                                    
#define DBG_IOMCURS                     5                          
#define DBG_IOMDESC                     6                               
#define DBG_IOPOWER                     7                              
#define DBG_IOSERVICE                   8                          
#define DBG_IOREGISTRY                  9                     

                                                      

#define DBG_IOSTORAGE           32                          
#define DBG_IONETWORK           33                          
#define DBG_IOKEYBOARD          34                    
#define DBG_IOHID               35                       
#define DBG_IOAUDIO             36                 
#define DBG_IOSERIAL            37                  
#define DBG_IOTTY               38                      
#define DBG_IOSAM               39                                          
#define DBG_IOPARALLELATA       40                        
#define DBG_IOPARALLELSCSI      41                         
#define DBG_IOSATA              42                      
#define DBG_IOSAS               43               
#define DBG_IOFIBRECHANNEL      44                        
#define DBG_IOUSB               45               
#define DBG_IOBLUETOOTH         46                     
#define DBG_IOFIREWIRE          47                    
#define DBG_IOINFINIBAND        48                      
#define DBG_IOCPUPM             49                                
#define DBG_IOGRAPHICS          50                    
#define DBG_HIBERNATE           51                                      
#define DBG_IOTHUNDERBOLT       52                       
#define DBG_BOOTER              53                                 
#define DBG_IOAUDIO2            54                            

#define DBG_IOSURFACEPA         64                                   
#define DBG_IOMDPA              65                                            
#define DBG_IODARTPA            66                              
                                                                       

                             
#define DBG_IOPOINTING          DBG_IOHID                                                            
#define DBG_IODISK                      DBG_IOSTORAGE                                                    

                                                                             
#define DBG_DRVSTORAGE        1                     
#define DBG_DRVNETWORK        2                     
#define DBG_DRVKEYBOARD       3               
#define DBG_DRVHID            4                  
#define DBG_DRVAUDIO          5            
#define DBG_DRVSERIAL         7             
#define DBG_DRVSAM            8                                     
#define DBG_DRVPARALLELATA    9                   
#define DBG_DRVPARALLELSCSI  10                    
#define DBG_DRVSATA          11                 
#define DBG_DRVSAS           12          
#define DBG_DRVFIBRECHANNEL  13                   
#define DBG_DRVUSB           14          
#define DBG_DRVBLUETOOTH     15                
#define DBG_DRVFIREWIRE      16               
#define DBG_DRVINFINIBAND    17                 
#define DBG_DRVGRAPHICS      18               
#define DBG_DRVSD            19                     
#define DBG_DRVNAND          20                              
#define DBG_SSD              21          
#define DBG_DRVSPI           22          
#define DBG_DRVWLAN_802_11   23                  
#define DBG_DRVSSM           24                                     
#define DBG_DRVSMC           25                                   
#define DBG_DRVMACEFIMANAGER 26                      
#define DBG_DRVANE           27          
#define DBG_DRVETHERNET      28               
#define DBG_DRVMCC           29                              
#define DBG_DRVACCESSORY     30                  

                             
#define DBG_DRVPOINTING         DBG_DRVHID                                            
#define DBG_DRVDISK             DBG_DRVSTORAGE                                            

                                                                          
#define DBG_DLIL_STATIC 1                             
#define DBG_DLIL_PR_MOD 2                                 
#define DBG_DLIL_IF_MOD 3                                  
#define DBG_DLIL_PR_FLT 4                                 
#define DBG_DLIL_IF_FLT 5                                  

                                                                
#define DBG_FSRW      0x1                                             
#define DBG_DKRW      0x2                                       
#define DBG_FSVN      0x3                                                    
#define DBG_FSLOOOKUP 0x4                                                    
#define DBG_JOURNAL   0x5                                
#define DBG_IOCTL     0x6                            
#define DBG_BOOTCACHE 0x7                               
#define DBG_HFS       0x8                                                   
#define DBG_APFS      0x9                                                     
#define DBG_SMB       0xA                                                   
#define DBG_MOUNT     0xB                                         
#define DBG_EXFAT     0xE                                                       
#define DBG_MSDOS     0xF                                                       
#define DBG_ACFS      0x10                                                      
#define DBG_THROTTLE  0x11                               
#define DBG_DECMP     0x12                                 
#define DBG_VFS       0x13                          
#define DBG_LIVEFS    0x14                                               
#define DBG_CONTENT_PROT 0xCF                                                        

  
                                                                      
   
#define DBG_HFS_UPDATE_ACCTIME   0x01
#define DBG_HFS_UPDATE_MODTIME   0x02
#define DBG_HFS_UPDATE_CHGTIME   0x04
#define DBG_HFS_UPDATE_MODIFIED  0x08
#define DBG_HFS_UPDATE_FORCE     0x10
#define DBG_HFS_UPDATE_DATEADDED 0x20
#define DBG_HFS_UPDATE_MINOR     0x40
#define DBG_HFS_UPDATE_SKIPPED   0x80

  
                                           
   
#define DBG_VFS_IO_COMPRESSION_STATS 0x1000

                                          
#define DBG_BSD_PROC              0x01                              
#define DBG_BSD_MEMSTAT           0x02                                       
#define DBG_BSD_KEVENT            0x03                              
#define DBG_BSD_EXCP_SC           0x0C                   
#define DBG_BSD_AIO               0x0D                           
#define DBG_BSD_SC_EXTENDED_INFO  0x0E                                  
#define DBG_BSD_SC_EXTENDED_INFO2 0x0F                                  
#define DBG_BSD_KDEBUG_TEST       0xFF                         

                                                  
#define BSD_PROC_EXIT              1                    
#define BSD_PROC_FRCEXIT           2                                
#define BSD_PROC_EXEC              3                            
#define BSD_PROC_EXITREASON_CREATE 4                            
#define BSD_PROC_EXITREASON_COMMIT 5                                      

                                                 
#define BSD_MEMSTAT_SCAN             1                                 
#define BSD_MEMSTAT_JETSAM           2                  
#define BSD_MEMSTAT_JETSAM_HIWAT     3                        
#define BSD_MEMSTAT_FREEZE           4                      
#define BSD_MEMSTAT_FREEZE_SCAN      5                                                
#define BSD_MEMSTAT_UPDATE           6                       
#define BSD_MEMSTAT_IDLE_DEMOTE      7                           
#define BSD_MEMSTAT_CLEAR_ERRORS     8                                     
#define BSD_MEMSTAT_DIRTY_TRACK      9                               
#define BSD_MEMSTAT_DIRTY_SET       10                             
#define BSD_MEMSTAT_DIRTY_CLEAR     11                               
#ifdef  PRIVATE
#define BSD_MEMSTAT_GRP_SET_PROP    12                            
#define BSD_MEMSTAT_DO_KILL         13                          
#define BSD_MEMSTAT_CHANGE_PRIORITY 14                        
#endif              
#define BSD_MEMSTAT_FAST_JETSAM     15                                            
#define BSD_MEMSTAT_COMPACTOR_RUN   16                                           
#define BSD_MEMSTAT_FREEZE_DISABLE  17                                                
#define BSD_MEMSTAT_RELAUNCH_FLAGS  18                                                                 

                                                
#define BSD_KEVENT_KQ_PROCESS_BEGIN   1
#define BSD_KEVENT_KQ_PROCESS_END     2
#define BSD_KEVENT_KQWQ_PROCESS_BEGIN 3
#define BSD_KEVENT_KQWQ_PROCESS_END   4
#define BSD_KEVENT_KQWQ_BIND          5
#define BSD_KEVENT_KQWQ_UNBIND        6
#define BSD_KEVENT_KQWQ_THREQUEST     7
#define BSD_KEVENT_KQWL_PROCESS_BEGIN 8
#define BSD_KEVENT_KQWL_PROCESS_END   9
#define BSD_KEVENT_KQWL_THREQUEST     10
#define BSD_KEVENT_KQWL_THADJUST      11
#define BSD_KEVENT_KQ_REGISTER        12
#define BSD_KEVENT_KQWQ_REGISTER      13
#define BSD_KEVENT_KQWL_REGISTER      14
#define BSD_KEVENT_KNOTE_ACTIVATE     15
#define BSD_KEVENT_KQ_PROCESS         16
#define BSD_KEVENT_KQWQ_PROCESS       17
#define BSD_KEVENT_KQWL_PROCESS       18
#define BSD_KEVENT_KQWL_BIND          19
#define BSD_KEVENT_KQWL_UNBIND        20
#define BSD_KEVENT_KNOTE_ENABLE       21
#define BSD_KEVENT_KNOTE_VANISHED     22

                                                
#define DBG_TRACE_DATA      0
#define DBG_TRACE_STRING    1
#define DBG_TRACE_INFO      2

                              
#define TRACE_DATA_NEWTHREAD            (TRACEDBG_CODE(DBG_TRACE_DATA, 1))
#define TRACE_DATA_EXEC                 (TRACEDBG_CODE(DBG_TRACE_DATA, 2))
#define TRACE_DATA_THREAD_TERMINATE     (TRACEDBG_CODE(DBG_TRACE_DATA, 3))
#define TRACE_DATA_THREAD_TERMINATE_PID (TRACEDBG_CODE(DBG_TRACE_DATA, 4))
#define TRACE_STRING_GLOBAL             (TRACEDBG_CODE(DBG_TRACE_STRING, 0))
#define TRACE_STRING_NEWTHREAD          (TRACEDBG_CODE(DBG_TRACE_STRING, 1))
#define TRACE_STRING_EXEC               (TRACEDBG_CODE(DBG_TRACE_STRING, 2))
#define TRACE_STRING_PROC_EXIT          (TRACEDBG_CODE(DBG_TRACE_STRING, 3))
#define TRACE_STRING_THREADNAME         (TRACEDBG_CODE(DBG_TRACE_STRING, 4))
#define TRACE_STRING_THREADNAME_PREV    (TRACEDBG_CODE(DBG_TRACE_STRING, 5))
#define TRACE_PANIC                     (TRACEDBG_CODE(DBG_TRACE_INFO, 0))
#define TRACE_TIMESTAMPS                (TRACEDBG_CODE(DBG_TRACE_INFO, 1))
#define TRACE_LOST_EVENTS               (TRACEDBG_CODE(DBG_TRACE_INFO, 2))
#define TRACE_WRITING_EVENTS            (TRACEDBG_CODE(DBG_TRACE_INFO, 3))
#define TRACE_INFO_STRING               (TRACEDBG_CODE(DBG_TRACE_INFO, 4))
#define TRACE_RETROGRADE_EVENTS         (TRACEDBG_CODE(DBG_TRACE_INFO, 5))

                                                      
#define DBG_CS_IO       0

                                                   
#define DBG_SEC_KERNEL  0                                          
#define DBG_SEC_SANDBOX 1

                                                                             

                                                    
#define DBG_MT_INSTRS_CYCLES 1
#define DBG_MT_DEBUG 2
#define DBG_MT_RESOURCES_PROC_EXIT 3
#define DBG_MT_RESOURCES_THR_EXIT 4
#define DBG_MT_TMPTH 0xfe
#define DBG_MT_TMPCPU 0xff

                                                                     
#define DBG_MT_INSTRS_CYCLES_PROC_EXIT MTDBG_RESOURCES_ON_PROC_EXIT(0)

                                                                    
#define DBG_MT_INSTRS_CYCLES_THR_EXIT  MTDBG_RESOURCES_ON_THR_EXIT(0)

                                               
#define DBG_MISC_COREBRIGHTNESS 0x01
#define DBG_MISC_VIDEOENG       0x02
#define DBG_EVENT               0x10
#define DBG_MISC_INSTRUMENTS    0x11
#define DBG_MISC_INSTRUMENTSBT  0x12
#define DBG_MISC_LAYOUT         0x1a
#define DBG_BUFFER              0x20

                                               
#define DBG_DYLD_UUID (5)

                                                       
#define DBG_DYLD_UUID_MAP_A             (0)
#define DBG_DYLD_UUID_MAP_B             (1)
#define DBG_DYLD_UUID_MAP_32_A          (2)
#define DBG_DYLD_UUID_MAP_32_B          (3)
#define DBG_DYLD_UUID_MAP_32_C          (4)
#define DBG_DYLD_UUID_UNMAP_A           (5)
#define DBG_DYLD_UUID_UNMAP_B           (6)
#define DBG_DYLD_UUID_UNMAP_32_A        (7)
#define DBG_DYLD_UUID_UNMAP_32_B        (8)
#define DBG_DYLD_UUID_UNMAP_32_C        (9)
#define DBG_DYLD_UUID_SHARED_CACHE_A    (10)
#define DBG_DYLD_UUID_SHARED_CACHE_B    (11)
#define DBG_DYLD_UUID_SHARED_CACHE_32_A (12)
#define DBG_DYLD_UUID_SHARED_CACHE_32_B (13)
#define DBG_DYLD_UUID_SHARED_CACHE_32_C (14)
#define DBG_DYLD_AOT_UUID_MAP_A         (15)
#define DBG_DYLD_AOT_UUID_MAP_B         (16)

                                                           
#define DKIO_DONE       0x01
#define DKIO_READ       0x02
#define DKIO_ASYNC      0x04
#define DKIO_META       0x08
#define DKIO_PAGING     0x10
#define DKIO_THROTTLE   0x20                                                           
#define DKIO_PASSIVE    0x40
#define DKIO_NOCACHE    0x80
#define DKIO_TIER_MASK  0xF00
#define DKIO_TIER_SHIFT 8
#define DKIO_TIER_UPGRADE 0x1000

                                                          
#define DBG_APP_LOGINWINDOW     0x03
#define DBG_APP_AUDIO           0x04
#define DBG_APP_SYSTEMUI        0x05
#define DBG_APP_SIGNPOST        0x0A
#define DBG_APP_APPKIT          0x0C
#define DBG_APP_UIKIT           0x0D
#define DBG_APP_DFR             0x0E
#define DBG_APP_LAYOUT          0x0F
#define DBG_APP_COREDATA        0x10
#define DBG_APP_SAMBA           0x80
#define DBG_APP_EOSSUPPORT      0x81
#define DBG_APP_MACEFIMANAGER   0x82

                                                      
#define OPEN_THROTTLE_WINDOW    0x1
#define PROCESS_THROTTLED       0x2
#define IO_THROTTLE_DISABLE     0x3
#define IO_TIER_UPL_MISMATCH    0x4

                                                              
                                           
#define IMP_ASSERTION                       0x10                                            
#define IMP_BOOST                           0x11                                  
#define IMP_MSG                             0x12                                                                 
#define IMP_WATCHPORT                       0x13                                                                                 
#define IMP_TASK_SUPPRESSION                0x17                                            
#define IMP_TASK_APPTYPE                    0x18                                    
#define IMP_UPDATE                          0x19                                            
#define IMP_USYNCH_QOS_OVERRIDE             0x1A                                                                                  
#define IMP_DONOR_CHANGE                    0x1B                                   
#define IMP_MAIN_THREAD_QOS                 0x1C                                            
#define IMP_SYNC_IPC_QOS                    0x1D                               
                                                                                 

                                
#define IMP_TASK_POLICY_DARWIN_BG           0x21
#define IMP_TASK_POLICY_IOPOL               0x22
#define IMP_TASK_POLICY_IO                  0x23
#define IMP_TASK_POLICY_PASSIVE_IO          0x24

                          
#define IMP_TASK_POLICY_DARWIN_BG_IOPOL     0x27
                                                   
#define IMP_TASK_POLICY_BOOST               0x29
#define IMP_TASK_POLICY_ROLE                0x2A
                                                   
#define IMP_TASK_POLICY_TERMINATED          0x2C
#define IMP_TASK_POLICY_NEW_SOCKETS_BG      0x2D
#define IMP_TASK_POLICY_SUP_ACTIVE          0x2E
#define IMP_TASK_POLICY_LATENCY_QOS         0x2F
#define IMP_TASK_POLICY_THROUGH_QOS         0x30
#define IMP_TASK_POLICY_WATCHERS_BG         0x31

#define IMP_TASK_POLICY_SFI_MANAGED         0x34
#define IMP_TASK_POLICY_ALL_SOCKETS_BG      0x37

#define IMP_TASK_POLICY_BASE_LATENCY_AND_THROUGHPUT_QOS  0x39                                              
#define IMP_TASK_POLICY_OVERRIDE_LATENCY_AND_THROUGHPUT_QOS  0x3A                                              

                            
#define IMP_TASK_POLICY_PIDBIND_BG          0x32
                                                   
                                                   
#define IMP_TASK_POLICY_QOS_OVERRIDE        0x36
#define IMP_TASK_POLICY_QOS_AND_RELPRIO     0x38                                                 
#define IMP_TASK_POLICY_QOS_WORKQ_OVERRIDE  0x3B
#define IMP_TASK_POLICY_QOS_PROMOTE         0x3C
#define IMP_TASK_POLICY_QOS_KEVENT_OVERRIDE 0x3D
#define IMP_TASK_POLICY_QOS_IPC_OVERRIDE    IMP_TASK_POLICY_QOS_KEVENT_OVERRIDE                  
#define IMP_TASK_POLICY_QOS_SERVICER_OVERRIDE 0x3E

                             
#define IMP_HOLD                0x2                                       
#define IMP_DROP                0x4                                       
#define IMP_EXTERN              0x8                                                                                       

                         
#define IMP_BOOSTED             0x1
#define IMP_UNBOOSTED           0x2                                       

                       
#define IMP_MSG_SEND            0x1                                                                  
#define IMP_MSG_DELV            0x2                                             

                          
#define IMP_UPDATE_TASK_CREATE  0x1

                                       
#define IMP_USYNCH_ADD_OVERRIDE         0x0                                                        
#define IMP_USYNCH_REMOVE_OVERRIDE      0x1                                                           

                                
#define IMP_DONOR_UPDATE_LIVE_DONOR_STATE       0x0
#define IMP_DONOR_INIT_DONOR_STATE              0x1

                               
#define IMP_SYNC_IPC_QOS_APPLIED                0x0
#define IMP_SYNC_IPC_QOS_REMOVED                0x1
#define IMP_SYNC_IPC_QOS_OVERFLOW               0x2
#define IMP_SYNC_IPC_QOS_UNDERFLOW              0x3

                                               
#define TURNSTILE_HEAP_OPERATIONS               0x10
#define TURNSTILE_PRIORITY_OPERATIONS           0x20
#define TURNSTILE_FREELIST_OPERATIONS           0x30

                                         
#define THREAD_ADDED_TO_TURNSTILE_WAITQ         0x1
#define THREAD_REMOVED_FROM_TURNSTILE_WAITQ     0x2
#define THREAD_MOVED_IN_TURNSTILE_WAITQ         0x3
#define TURNSTILE_ADDED_TO_TURNSTILE_HEAP       0x4
#define TURNSTILE_REMOVED_FROM_TURNSTILE_HEAP   0x5
#define TURNSTILE_MOVED_IN_TURNSTILE_HEAP       0x6
#define TURNSTILE_ADDED_TO_THREAD_HEAP          0x7
#define TURNSTILE_REMOVED_FROM_THREAD_HEAP      0x8
#define TURNSTILE_MOVED_IN_THREAD_HEAP          0x9
#define TURNSTILE_UPDATE_STOPPED_BY_LIMIT       0xa
#define THREAD_NOT_WAITING_ON_TURNSTILE         0xb

                                             
#define TURNSTILE_PRIORITY_CHANGE               0x1
#define THREAD_USER_PROMOTION_CHANGE            0x2

                                             
#define TURNSTILE_PREPARE                       0x1
#define TURNSTILE_COMPLETE                      0x2

                                                                   
#define BANK_ACCOUNT_INFO               0x10                                                     
#define BANK_TASK_INFO                  0x11                                                  

                                                             
#define ATM_SUBAID_INFO                 0x10
#define ATM_GETVALUE_INFO               0x20
#define ATM_UNREGISTER_INFO             0x30

                                 
#define BANK_SETTLE_CPU_TIME            0x1                                                
#define BANK_SECURE_ORIGINATOR_CHANGED  0x2                                     
#define BANK_SETTLE_ENERGY              0x3                                                        

                               
#define ATM_MIN_CALLED                          0x1
#define ATM_LINK_LIST_TRIM                      0x2

                                 
#define ATM_VALUE_REPLACED                      0x1
#define ATM_VALUE_ADDED                         0x2

                                   
#define ATM_VALUE_UNREGISTERED                  0x1
#define ATM_VALUE_DIFF_MAILBOX                  0x2

                                                       
#define DBG_DAEMON_COREDUET 0x1
#define DBG_DAEMON_POWERD   0x2

                                             
#define DBG_UMALLOC_EXTERNAL                    0x1
#define DBG_UMALLOC_INTERNAL                    0x2

                                                                        

#define KDBG_MIGCODE(msgid) (((unsigned)DBG_MIG << KDBG_CLASS_OFFSET) | \
	                     ((unsigned)((msgid) & 0x3fffff) << KDBG_CODE_OFFSET))

#define MACHDBG_CODE(SubClass, code) KDBG_CODE(DBG_MACH, SubClass, code)
#define NETDBG_CODE(SubClass, code) KDBG_CODE(DBG_NETWORK, SubClass, code)
#define FSDBG_CODE(SubClass, code) KDBG_CODE(DBG_FSYSTEM, SubClass, code)
#define BSDDBG_CODE(SubClass, code) KDBG_CODE(DBG_BSD, SubClass, code)
#define IOKDBG_CODE(SubClass, code) KDBG_CODE(DBG_IOKIT, SubClass, code)
#define DRVDBG_CODE(SubClass, code) KDBG_CODE(DBG_DRIVERS, SubClass, code)
#define TRACEDBG_CODE(SubClass, code) KDBG_CODE(DBG_TRACE, SubClass, code)
#define SILICONDBG_CODE(SubClass, code) KDBG_CODE(DBG_SILICON, SubClass, code)
#define MISCDBG_CODE(SubClass, code) KDBG_CODE(DBG_MISC, SubClass, code)
#define DLILDBG_CODE(SubClass, code) KDBG_CODE(DBG_DLIL, SubClass, code)
#define SECURITYDBG_CODE(SubClass, code) KDBG_CODE(DBG_SECURITY, SubClass, code)
#define DYLDDBG_CODE(SubClass, code) KDBG_CODE(DBG_DYLD, SubClass, code)
#define QTDBG_CODE(SubClass, code) KDBG_CODE(DBG_QT, SubClass, code)
#define APPSDBG_CODE(SubClass, code) KDBG_CODE(DBG_APPS, SubClass, code)
#define ARIADNEDBG_CODE(SubClass, code) KDBG_CODE(DBG_ARIADNE, SubClass, code)
#define DAEMONDBG_CODE(SubClass, code) KDBG_CODE(DBG_DAEMON, SubClass, code)
#define CPUPM_CODE(code) IOKDBG_CODE(DBG_IOCPUPM, code)
#define MTDBG_CODE(SubClass, code) KDBG_CODE(DBG_MONOTONIC, SubClass, code)
#define MTDBG_RESOURCES_ON_PROC_EXIT(code) MTDBG_CODE(DBG_MT_RESOURCES_PROC_EXIT, code)
#define MTDBG_RESOURCES_ON_THR_EXIT(code) MTDBG_CODE(DBG_MT_RESOURCES_THR_EXIT, code)

#define KMEM_ALLOC_CODE MACHDBG_CODE(DBG_MACH_LEAKS, 0)
#define KMEM_ALLOC_CODE_2 MACHDBG_CODE(DBG_MACH_LEAKS, 1)
#define KMEM_FREE_CODE MACHDBG_CODE(DBG_MACH_LEAKS, 2)
#define KMEM_FREE_CODE_2 MACHDBG_CODE(DBG_MACH_LEAKS, 3)
#define ZALLOC_CODE MACHDBG_CODE(DBG_MACH_LEAKS, 4)
#define ZALLOC_CODE_2 MACHDBG_CODE(DBG_MACH_LEAKS, 5)
#define ZFREE_CODE MACHDBG_CODE(DBG_MACH_LEAKS, 6)
#define ZFREE_CODE_2 MACHDBG_CODE(DBG_MACH_LEAKS, 7)

#define PMAP_CODE(code) MACHDBG_CODE(DBG_MACH_PMAP, code)

#define IMPORTANCE_CODE(SubClass, code) KDBG_CODE(DBG_IMPORTANCE, (SubClass), (code))
#define BANK_CODE(SubClass, code) KDBG_CODE(DBG_BANK, (SubClass), (code))
#define ATM_CODE(SubClass, code) KDBG_CODE(DBG_ATM, (SubClass), (code))
#define TURNSTILE_CODE(SubClass, code) KDBG_CODE(DBG_TURNSTILE, (SubClass), (code))

                                              
#define COREDUETDBG_CODE(code) DAEMONDBG_CODE(DBG_DAEMON_COREDUET, code)
#define POWERDDBG_CODE(code) DAEMONDBG_CODE(DBG_DAEMON_POWERD, code)

                                         
#define VFS_LOOKUP      (FSDBG_CODE(DBG_FSRW,36))
#define VFS_LOOKUP_DONE (FSDBG_CODE(DBG_FSRW,39))

#endif                           

__END_DECLS

#if defined(__has_include) && __has_include(<sys/kdebug_private.h>)
#include <sys/kdebug_private.h>
#endif                                            

#ifdef KERNEL
#include <sys/kdebug_kernel.h>
#endif                      

#endif                                 
