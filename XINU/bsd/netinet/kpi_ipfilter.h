/*
 * Copyright (c) 2008-2017 Apple Inc. All rights reserved.
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
   
                               
                                                                           
                                                                           
                                                                         
                   
   

#ifndef __KPI_IPFILTER__
#define __KPI_IPFILTER__

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15))
#else
#define __NKE_API_DEPRECATED
#endif              

  
              
  
                                                                      
                         
   
struct ipf_pktopts {
	u_int32_t                       ippo_flags;
	ifnet_t                         ippo_mcast_ifnet;
	int                             ippo_mcast_loop;
	u_int8_t                        ippo_mcast_ttl;
};
#define IPPOF_MCAST_OPTS            0x1
#ifdef PRIVATE
#define IPPOF_BOUND_IF              0x2
#define IPPOF_NO_IFT_CELLULAR       0x4
#define IPPOF_SELECT_SRCIF          0x8
#define IPPOF_BOUND_SRCADDR         0x10
#define IPPOF_SHIFT_IFSCOPE         16
#define IPPOF_NO_IFF_EXPENSIVE      0x20
#define IPPOF_NO_IFF_CONSTRAINED    0x40
#endif              

typedef struct ipf_pktopts *ipf_pktopts_t;

__BEGIN_DECLS

   
                                
  
                                                                          
                                                                             
                                                                           
                                                                           
                                                                                
                                                                               
                                                                      
                                                                               
                                                                               
                                                                        
                                                                            
                                          
                                                                          
                                                                         
                        
                                                   
                                           
                                                                                 
                        
                                                                           
                                
                                                                          
                                                      
                                                                         
                                    
   
typedef errno_t (*ipf_input_func)(void *cookie, mbuf_t *data, int offset,
    u_int8_t protocol);

   
                                 
  
                                                                           
                                                                           
                                                                               
                                                                    
                                                          
                                                                          
                                                                             
                                       
                        
                                                                           
                                
                                                                          
                                                      
                                                                         
                                    
   
typedef errno_t (*ipf_output_func)(void *cookie, mbuf_t *data,
    ipf_pktopts_t options);

   
                                 
  
                                                                            
                                   
                                                                          
   
typedef void (*ipf_detach_func)(void *cookie);

   
                            
                                                                      
                                                              
                                                                       
                                  
                                                               
                                                                        
                                                                          
                                                                     
   
struct ipf_filter {
	void            *cookie;
	const char      *name;
	ipf_input_func  ipf_input;
	ipf_output_func ipf_output;
	ipf_detach_func ipf_detach;
};

struct opaque_ipfilter;
typedef struct opaque_ipfilter *ipfilter_t;

   
                            
                                                
                                                       
                                                                       
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t ipf_addv4_internal(const struct ipf_filter *filter,
    ipfilter_t *filter_ref);

#define ipf_addv4(filter, filter_ref) \
    ipf_addv4_internal((filter), (filter_ref))
#else
extern errno_t ipf_addv4(const struct ipf_filter *filter,
    ipfilter_t *filter_ref)
__NKE_API_DEPRECATED;
#endif                     

   
                            
                                                
                                                       
                                                                       
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t ipf_addv6_internal(const struct ipf_filter *filter,
    ipfilter_t *filter_ref);

#define ipf_addv6(filter, filter_ref) \
    ipf_addv6_internal((filter), (filter_ref))
#else
extern errno_t ipf_addv6(const struct ipf_filter *filter,
    ipfilter_t *filter_ref)
__NKE_API_DEPRECATED;
#endif                     

   
                             
                                                     
                                                                                 
                           
                                                        
   
extern errno_t ipf_remove(ipfilter_t filter_ref)
__NKE_API_DEPRECATED;

   
                                   
                                                                   
                                                                                
                                                                              
                                                                          
                                                                      
                                                                               
                                                                               
                                                            
  
                                                                               
                                                   
                                                                             
                        
                                                                         
                                                        
   
extern errno_t ipf_inject_input(mbuf_t data, ipfilter_t filter_ref)
__NKE_API_DEPRECATED;

   
                                    
                                                                           
                                                                         
                                                                        
                                                                          
                                                                                
                                                                                 
                                                                             
                                                  
                                                      
                                                                         
                                                     
                                                                          
                                           
   
extern errno_t ipf_inject_output(mbuf_t data, ipfilter_t filter_ref,
    ipf_pktopts_t options)
__NKE_API_DEPRECATED;

__END_DECLS
#undef __NKE_API_DEPRECATED
#endif                       
