/*
 * Copyright (c) 2015-2017 Apple Inc. All rights reserved.
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

#ifndef _SYS_WORK_INTERVAL_H
#define _SYS_WORK_INTERVAL_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/_types/_size_t.h>

#include <mach/port.h>

__BEGIN_DECLS

  
                                                                  
                               
  
                                                                      
                                                                      
                                                                           
                                                                    
                                                                         
                                                                           
        
  
                                                                    
                                          
  
                                                     
                                             
                                             
  
                                 
                          
                                          
                                  
  
                                            
                                       
                                       
  
               
  
                                                                            
                                                                       
                                                                   
                                                                           
                                    
                                                                            
                                                                           
                                        
                                                                              
                                                                            
                                                                       
                                              
                                                                      
                                                                      
                                                                            
                                                                            
                         
  
             
  
                                                                          
                                                                            
  
                                                                
                                                                      
                                                            
               
  
               
  
                                                                                 
                                                                               
                                                                         
  
                                   
  
                                                                              
                                                                       
                                                                        
                                                                        
                                                                       
                                                                      
                                                                      
                                                                         
                                                                     
                                                                          
                                                                       
                                                                      
                                                         
  
                                         
  
                                                                                 
                                                                                  
                                                                           
                                                                                   
                                                                               
                                                                              
                                                                            
                                                                               
                                                                             
                                                                                   
                                                                               
                                                                                   
                                                         
   

                                                    

                                                                                                 
#define WORK_INTERVAL_FLAG_JOINABLE                     (0x1)
                                                                                                             
#define WORK_INTERVAL_FLAG_GROUP                        (0x2)
                                                                            
                                                                              
                                                                              
                 
#define WORK_INTERVAL_FLAG_UNRESTRICTED                 (0x4)

                                                                                         
#define WORK_INTERVAL_FLAG_ENABLE_AUTO_JOIN             (0x8)
                                                                                               
#define WORK_INTERVAL_FLAG_ENABLE_DEFERRED_FINISH       (0x10)

                                                                        
#define WORK_INTERVAL_FLAG_IGNORED                      (0x20)

                                                                                          
                                                                  
#define WORK_INTERVAL_FLAG_FINISH_AT_DEADLINE           (0x40)

                                                                         
#define WORK_INTERVAL_TYPE_MASK                 (0xF0000000)
#define WORK_INTERVAL_TYPE_DEFAULT              (0x0 << 28)
#define WORK_INTERVAL_TYPE_COREAUDIO            (0x1 << 28)
#define WORK_INTERVAL_TYPE_COREANIMATION        (0x2 << 28)
#define WORK_INTERVAL_TYPE_CA_RENDER_SERVER     (0x2 << 28)
#define WORK_INTERVAL_TYPE_CA_CLIENT            (0x3 << 28)
#define WORK_INTERVAL_TYPE_HID_DELIVERY         (0x4 << 28)
#define WORK_INTERVAL_TYPE_COREMEDIA            (0x5 << 28)
#define WORK_INTERVAL_TYPE_ARKIT                (0x6 << 28)
#define WORK_INTERVAL_TYPE_LAST                 (0xF << 28)

#ifndef KERNEL

typedef struct work_interval *work_interval_t;
typedef struct work_interval_instance *work_interval_instance_t;

  
                                     
  
                                                                           
  
                 
                                                           
                                                         
                                                 
  
                         
                                                
                                          
                                                        
  
                      
                                                                     
                                                                   
                                                                             
  
                                                                        
                                                          
  
                                                                                         
  
                                                                                              
   
int     work_interval_create(work_interval_t *interval_handle, uint32_t flags);

                                                                    
  
                                                              
                           
   
int work_interval_get_flags_from_port(mach_port_t port, uint32_t *flags);


  
                                                                                           
  
                                                              
   
int     work_interval_notify(work_interval_t    interval_handle,
    uint64_t start, uint64_t finish,
    uint64_t deadline, uint64_t next_start,
    uint32_t flags);

  
                                                           
  
                                                              
   
int     work_interval_notify_simple(work_interval_t interval_handle,
    uint64_t start, uint64_t deadline,
    uint64_t next_start);

  
                                  
                                                           
                                                               
   
int     work_interval_destroy(work_interval_t interval_handle);

  
                                              
                                                                 
  
                                                            
   
int     work_interval_join(work_interval_t interval_handle);

  
                                                                  
                                                                                  
                                                                  
  
                                                            
   
int     work_interval_copy_port(work_interval_t interval_handle, mach_port_t *port);

  
                                         
  
                                                                                          
                                                                               
  
                                                                        
  
                                                            
  
                                                                                   
                            
   
int     work_interval_join_port(mach_port_t port);

  
                                           
   
int     work_interval_leave(void);

#endif              

#if PRIVATE

                                                  
#define WORK_INTERVAL_OPERATION_CREATE  0x00000001                      
#define WORK_INTERVAL_OPERATION_DESTROY 0x00000002                       
#define WORK_INTERVAL_OPERATION_NOTIFY  0x00000003                                                 
#define WORK_INTERVAL_OPERATION_CREATE2 0x00000004                                                
#define WORK_INTERVAL_OPERATION_JOIN    0x00000005                              
#define WORK_INTERVAL_OPERATION_GET_FLAGS 0x00000009                            

struct work_interval_notification {
	uint64_t        start;
	uint64_t        finish;
	uint64_t        deadline;
	uint64_t        next_start;
	uint32_t        notify_flags;
	uint32_t        create_flags;
};
typedef struct work_interval_notification *work_interval_notification_t;

struct work_interval_create_params {
	uint64_t        wicp_id;                            
	mach_port_name_t wicp_port;                         
	uint32_t        wicp_create_flags;
};


int     __work_interval_ctl(uint32_t operation, uint64_t work_interval_id, void *arg, size_t len);

#endif              

__END_DECLS

#endif                           
