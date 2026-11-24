/*
 * Copyright (c) 2003,2008,2017 Apple Computer, Inc. All rights reserved.
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
   
                                      
                                                                          
                                                                         
                                                                          
                                                                          
                
   

#ifndef __KPI_INTERFACEFILTER__
#define __KPI_INTERFACEFILTER__
#include <sys/kernel_types.h>
#include <net/kpi_interface.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15.4))
#else
#define __NKE_API_DEPRECATED
#endif              

struct kev_msg;

__BEGIN_DECLS

   
                                
  
                                                                           
                                                                                
                                                                               
                                                                             
                                                                               
                                                                            
                                                                               
                                                                            
                        
  
                                                                             
                                                                                 
                                                                                 
                                                                                 
                                                                                 
                                                                             
                                                                 
                                                                          
                                                                   
                                                                        
                                                                                 
                                               
                                                                             
                                  
                                                                           
                                                                               
                                                    
                        
                                                                           
                                
                                                                          
                                                      
                                                                         
                                    
   
typedef errno_t (*iff_input_func)(void *cookie, ifnet_t interface,
    protocol_family_t protocol, mbuf_t *data, char **frame_ptr);

   
                                 
  
                                                                            
                                                                             
                                                                               
                                                                   
                                                                          
                                                                           
                                                                          
                                                                              
                                                                    
                        
                                                                           
                                
                                                                          
                                                      
                                                                         
                                    
   
typedef errno_t (*iff_output_func)(void *cookie, ifnet_t interface,
    protocol_family_t protocol, mbuf_t *data);

   
                                
  
                                                                        
                                                                            
                                                                               
                                                                   
                                                                          
                                                                           
                                                               
   
typedef void (*iff_event_func)(void *cookie, ifnet_t interface,
    protocol_family_t protocol, const struct kev_msg *event_msg);

   
                                
  
                                                                       
                                                                               
                                                                               
                                                                   
  
                                                                              
                                                                              
                                               
                                                                          
                                                                           
                                            
                                                          
                        
                                                            
                                                                              
                                           
                                                                      
                                                
                                                                        
                                  
   
typedef errno_t (*iff_ioctl_func)(void *cookie, ifnet_t interface,
    protocol_family_t protocol, unsigned long ioctl_cmd, void *ioctl_arg);

   
                                   
  
                                                                             
                                                                              
                                                                              
                                                                               
                                                                                
                                                                                 
                                                                  
                                                                          
                                                                      
   
typedef void (*iff_detached_func)(void *cookie, ifnet_t interface);

   
                           
                                                                             
                                                   
                                                                           
                                  
                                                                   
                                                                       
                                                                               
                                              
                                                                            
                         
                                                                          
                             
                                                                             
                         
                                                                             
                         
                                                                               
                                      
   

struct iff_filter {
	void                    *iff_cookie;
	const char              *iff_name;
	protocol_family_t       iff_protocol;
	iff_input_func          iff_input;
	iff_output_func         iff_output;
	iff_event_func          iff_event;
	iff_ioctl_func          iff_ioctl;
	iff_detached_func       iff_detached;
};

   
                              
                                                                  
                                                                         
                                                       
                                                                    
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t iflt_attach_internal(ifnet_t interface, const struct iff_filter *filter,
    interface_filter_t *filter_ref);

#define iflt_attach(interface, filter, filter_ref) \
	iflt_attach_internal((interface), (filter), (filter_ref))
#else
extern errno_t iflt_attach(ifnet_t interface, const struct iff_filter *filter,
    interface_filter_t *filter_ref)
__NKE_API_DEPRECATED;
#endif                     

   
                              
                                                                    
                                                                        
   
extern void iflt_detach(interface_filter_t filter_ref)
__NKE_API_DEPRECATED;

__END_DECLS
#undef __NKE_API_DEPRECATED
#endif                              
