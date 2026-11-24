/*
 * Copyright (c) 2000-2014 Apple Inc. All rights reserved.
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
   
                             
                                                                                
                                                                           
                
   

#ifndef SYS_KERN_EVENT_H
#define SYS_KERN_EVENT_H

#include <sys/appleapiopts.h>
#include <sys/ioccom.h>
#include <sys/sys_domain.h>

#define KEV_SNDSPACE    (4 * 1024)
#define KEV_RECVSPACE   (32 * 1024)

#define KEV_ANY_VENDOR          0
#define KEV_ANY_CLASS           0
#define KEV_ANY_SUBCLASS        0

  
              
   

   
                                  
                                                                                 
                                                                                 
                                                                           
   
#define KEV_VENDOR_APPLE        1

  
                                                               
   

   
                                   
                                                
   
#define KEV_NETWORK_CLASS       1

   
                                 
                                              
   
#define KEV_IOKIT_CLASS         2

   
                                  
                                               
   
#define KEV_SYSTEM_CLASS        3

   
                                      
                                                   
   
#define KEV_APPLESHARE_CLASS    4

   
                                    
                                                 
   
#define KEV_FIREWALL_CLASS      5

   
                                     
                                                    
   
#define KEV_IEEE80211_CLASS     6

  
                                                                             
                                                                            
                                                                         
                                                             
                                                                               
                                                                         
                                                                                
                                                                
   
#if defined(XNU_KERN_EVENT_DATA_IS_VLA)
#define XNU_KERN_EVENT_DATA_SIZE                          
#else
#define XNU_KERN_EVENT_DATA_SIZE 1
#endif

   
                               
                                                                           
                                                                              
                                                                             
                                                                             
                                                      
                                                                               
                        
                                                                                
                                                                              
                                                                               
                                                                      
                                  
                                                        
                                                              
                                                  
                                          
                                                                            
                                                                        
                                                                           
                                                        
   
struct kern_event_msg {
	u_int32_t       total_size;                                   
	u_int32_t       vendor_code;                                     
	u_int32_t       kev_class;                                 
	u_int32_t       kev_subclass;                               
	u_int32_t       id;                                                 
	u_int32_t       event_code;                      
	u_int32_t       event_data[XNU_KERN_EVENT_DATA_SIZE];                               
};

   
                                     
                                                                                
                                                                              
                                                                            
                                                             
   
#define KEV_MSG_HEADER_SIZE (offsetof(struct kern_event_msg, event_data[0]))

   
                            
                                                                     
                                                                             
                                       
                                                                               
                        
                                                                               
                                                                              
                                             
                                                                               
                                                                                 
                           
                                                                             
                                                                                
                                          
   
struct kev_request {
	u_int32_t       vendor_code;
	u_int32_t       kev_class;
	u_int32_t       kev_subclass;
};

   
                                             
                                                                                
                                                                                 
   
#define KEV_VENDOR_CODE_MAX_STR_LEN     200

   
                                
                                                                            
                                                                           
                                                               
                                                                                 
                                                                   
                                                        
   
#pragma pack(4)
struct kev_vendor_code {
	u_int32_t       vendor_code;
	char            vendor_string[KEV_VENDOR_CODE_MAX_STR_LEN];
};
#pragma pack()

   
                            
                                                                             
                                                                              
                         
   
#define SIOCGKEVID      _IOR('e', 1, u_int32_t)

   
                              
                                                                               
                                                                              
   
#define SIOCSKEVFILT    _IOW('e', 2, struct kev_request)

   
                              
                                                                             
                                                                             
                        
   
#define SIOCGKEVFILT    _IOR('e', 3, struct kev_request)

   
                                
                                                                                 
                                                                             
                       
   
#define SIOCGKEVVENDOR  _IOWR('e', 4, struct kev_vendor_code)

#ifdef PRIVATE
struct xkevtpcb {
	u_int32_t       kep_len;
	u_int32_t       kep_kind;
	u_int64_t       kep_evtpcb;
	u_int32_t       kep_vendor_code_filter;
	u_int32_t       kep_class_filter;
	u_int32_t       kep_subclass_filter;
};

struct kevtstat {
	u_int64_t       kes_pcbcount __attribute__((aligned(8)));
	u_int64_t       kes_gencnt __attribute__((aligned(8)));
	u_int64_t       kes_badvendor __attribute__((aligned(8)));
	u_int64_t       kes_toobig __attribute__((aligned(8)));
	u_int64_t       kes_nomem __attribute__((aligned(8)));
	u_int64_t       kes_fullsock __attribute__((aligned(8)));
	u_int64_t       kes_posted __attribute__((aligned(8)));
};
#endif              

#ifdef KERNEL
   
                              
                                                                            
   
#define N_KEV_VECTORS   5

   
                              
                                                                           
                       
                                               
                                           
   
struct kev_d_vectors {
	u_int32_t       data_length;                                  
	void            *data_ptr;                                 
};

   
                        
                                                                        
                                                                             
                                            
                                               
                                           
                                                                                
                                     
   
struct kev_msg {
	u_int32_t vendor_code;                                           
	u_int32_t kev_class;                                       
	u_int32_t kev_subclass;                                     
	u_int32_t event_code;                               
	struct kev_d_vectors dv[N_KEV_VECTORS];                           
};

   
                                       
                                                                              
                                                                              
                                                                            
                                               
                                                                               
                                                                          
                                       
                                                                                
                                 
   
errno_t kev_vendor_code_find(const char *vendor_string, u_int32_t *vendor_code);

   
                               
                                                 
                                                                                
                                                                             
                                                                              
                                                                             
                                                                              
                                                                        
   
errno_t kev_msg_post(struct kev_msg *event_msg);

#ifdef PRIVATE
  
                                                                            
          
   
int     kev_post_msg(struct kev_msg *event);

LIST_HEAD(kern_event_head, kern_event_pcb);

struct kern_event_pcb {
	decl_lck_mtx_data(, evp_mtx);                         
	LIST_ENTRY(kern_event_pcb) evp_link;                                  
	struct socket *evp_socket;                                          
	u_int32_t evp_vendor_code_filter;
	u_int32_t evp_class_filter;
	u_int32_t evp_subclass_filter;
};

#define sotoevpcb(so)   ((struct kern_event_pcb *)((so)->so_pcb))

#endif              
#endif             
#endif                       
