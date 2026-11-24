/*
 * Copyright (c) 2008-2016 Apple Inc. All rights reserved.
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
   
                               
                                                                             
                                                                            
                                                                           
                                                                            
                                    
   

#ifndef __KPI_PROTOCOL__
#define __KPI_PROTOCOL__
#include <sys/kernel_types.h>
#include <net/kpi_interface.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15.4))
#else
#define __NKE_API_DEPRECATED
#endif              

__BEGIN_DECLS

                                                                                
                                                                                
                                                                                

#ifdef BSD_KERNEL_PRIVATE
   
                                        
                                                                           
                                                                                 
                                              
                                                                   
                                                       
   
typedef void (*proto_input_handler)(protocol_family_t protocol, mbuf_t packet);

   
                                              
                                                                         
                                                                          
                                                                                 
                                   
                                                
   
typedef void (*proto_input_detached_handler)(protocol_family_t protocol);

   
                                       
                                                                             
                                                   
                                                                         
                             
                                                                 
                                                             
                                          
   
extern errno_t proto_register_input(protocol_family_t protocol,
    proto_input_handler input, proto_input_detached_handler detached,
    int chains);

   
                                         
                                                                         
                                                                                
                                                                               
                                                                                 
                                                                               
                                                                      
                                                                         
                             
   
extern void proto_unregister_input(protocol_family_t protocol);
#endif                         

   
                              
                                                                             
                      
                                                    
                                                                          
                                                                           
                                                                
   
extern errno_t proto_input(protocol_family_t protocol, mbuf_t packet)
__NKE_API_DEPRECATED;

   
                               
                                                                    
                                                                                 
                                            
                                                    
                                                                             
                                                                            
                                                                
   
extern errno_t proto_inject(protocol_family_t protocol, mbuf_t packet)
__NKE_API_DEPRECATED;


                                                                                
                                                                                
                                                                                

   
                                     
                                                                             
                                                                               
                                                                         
                                                                     
                                                                    
                           
                
                                                       
   
typedef errno_t (*proto_plumb_handler)(ifnet_t ifp, protocol_family_t protocol);

   
                                       
                                                                         
                                                                         
                                                                         
                                                                       
                                                                      
                           
   
typedef void (*proto_unplumb_handler)(ifnet_t ifp, protocol_family_t protocol);

   
                                         
                                                                             
                                                                                
                                                           
                                                                           
                        
                                                                         
                        
                                                                       
                           
                                                                         
                                                                                
                                                
                                                       
   
extern errno_t proto_register_plumber(protocol_family_t proto_fam,
    ifnet_family_t if_fam, proto_plumb_handler plumb,
    proto_unplumb_handler unplumb)
__NKE_API_DEPRECATED;

   
                                           
                                                                           
                                                                      
                        
                                                                            
   
extern void proto_unregister_plumber(protocol_family_t proto_fam,
    ifnet_family_t if_fam)
__NKE_API_DEPRECATED;

#ifdef BSD_KERNEL_PRIVATE
  
                              
                                                                              
                                                                           
                                                                               
                                        
                                                    
                                                           
                             
                                                  
                                                                   
   
extern errno_t proto_plumb(protocol_family_t protocol_family, ifnet_t ifp);

  
                                
                                                                           
                                                                           
                                                                           
                                                                         
                                                                    
                                                    
                                                               
                             
                                                  
                                                                   
   
extern errno_t proto_unplumb(protocol_family_t protocol_family, ifnet_t ifp);

__private_extern__ void
proto_kpi_init(void);

#endif                         
__END_DECLS

#undef __NKE_API_DEPRECATED
#endif                       
