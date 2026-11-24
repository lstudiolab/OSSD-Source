/*
 * Copyright (c) 2008-2017 Apple Computer, Inc. All rights reserved.
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
   
                                   
                                                                          
                      
  
                                                                          
                                                                            
                                                                           
                                                                            
                                                                            
                                                                      
                                                                       
                                                                          
                                                                            
                                                                          
                                                                          
                                                                             
                                                                             
                                                                            
                                                                        
                                                                             
                                                                            
                                                           
   
#ifndef __KPI_SOCKETFILTER__
#define __KPI_SOCKETFILTER__

#include <sys/kernel_types.h>
#include <sys/kpi_socket.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15))
#else
#define __NKE_API_DEPRECATED
#endif              

struct sockaddr;

   
                         
                                                                             
                                         
                                                                     
                                                                  
                                                                            
                                                                             
                        
                                                                           
                                                                      
                                                                           
                                                                          
                                                                              
   
enum {
	SFLT_GLOBAL             = 0x01,
	SFLT_PROG               = 0x02,
	SFLT_EXTENDED           = 0x04,
	SFLT_EXTENDED_REGISTRY  = 0x08
};
typedef u_int32_t       sflt_flags;

   
                             
                                                                            
                                                           
   
typedef u_int32_t       sflt_handle;

   
                           
                                                                            
                                                                               
                                               
                                                                            
                                 
                                                                           
                                        
                                                                      
                         
                                                                         
                                                                          
                                                                                 
                                                       
                                                                            
                           
                                                                         
                           
                                                                    
                                                                        
                                                                
   
enum {
	sock_evt_connecting             = 1,
	sock_evt_connected              = 2,
	sock_evt_disconnecting          = 3,
	sock_evt_disconnected           = 4,
	sock_evt_flush_read             = 5,
	sock_evt_shutdown               = 6,                                                                                          
	sock_evt_cantrecvmore           = 7,
	sock_evt_cantsendmore           = 8,
	sock_evt_closing                = 9,
	sock_evt_bound                  = 10
};
typedef u_int32_t       sflt_event_t;

   
                               
                                                                    
                                                                                
                                                                                
                                                                             
                      
                                                                      
                                                                     
   
enum {
	sock_data_filt_flag_oob         = 1,
	sock_data_filt_flag_record      = 2
};
typedef u_int32_t       sflt_data_flag_t;

__BEGIN_DECLS

   
                                      
  
                                                                           
                                                                                
                                                                          
                                                                           
                                                                       
                                                                             
   
typedef void (*sf_unregistered_func)(sflt_handle handle);

   
                                
  
                                                                         
                                                                              
                                                                               
                                            
                                                                                  
                                                                                    
                               
                                                                            
                                 
                                                              
                                                                        
                                         
   
typedef errno_t (*sf_attach_func)(void  **cookie, socket_t so);

   
                                
  
                                                                         
                                                                                
                                                                            
                                                          
                                                                        
                        
                                                        
                                                                            
                                         
   
typedef void (*sf_detach_func)(void *cookie, socket_t so);

   
                                
  
                                                                             
                                                                       
                                                                        
                        
                                                        
                                                          
                                                             
   
typedef void (*sf_notify_func)(void *cookie, socket_t so, sflt_event_t event,
    void *param);

   
                                     
  
                                                                       
                                                                            
                                                                         
                                                                              
                                                                            
                         
                                                                        
                        
                                                        
                                                         
                                                                         
                                                                          
                                                                       
                             
   
typedef int (*sf_getpeername_func)(void *cookie, socket_t so,
    struct sockaddr **sa);

   
                                     
  
                                                                       
                                                                            
                                                                         
                                                                              
                                                                            
                         
                                                                        
                        
                                                        
                                                         
                                                                         
                                                                          
                                                                       
                             
   
typedef int (*sf_getsockname_func)(void *cookie, socket_t so,
    struct sockaddr **sa);

   
                                 
  
                                                                               
                                                                            
                                                                         
                                                                              
                                                                              
                                                                             
                                                                        
                        
                                                        
                                                                           
                              
                                                                            
                                                                            
                      
                                                                           
                                                                        
                        
                        
                                                                                 
                                                                            
                                                
                                                                       
                                    
   
typedef errno_t (*sf_data_in_func)(void *cookie, socket_t so,
    const struct sockaddr *from, mbuf_t *data, mbuf_t *control,
    sflt_data_flag_t flags);

   
                                  
  
                                                                           
                                                                                 
                                                                         
                                                                             
                      
                                                                        
                        
                                                        
                                                                        
                              
                                                                            
                                                                            
                      
                                                                           
                                                                        
                        
                        
                                                                                 
                                                                        
                                                    
                                                                       
                                    
   
typedef errno_t (*sf_data_out_func)(void *cookie, socket_t so,
    const struct sockaddr *to, mbuf_t *data, mbuf_t *control,
    sflt_data_flag_t flags);

   
                                    
  
                                                                                
                                                                       
                                                                                 
                                                                                
                                                              
                                                                        
                        
                                                        
                                                                 
                        
                                                                           
                                    
                                                                       
                                    
   
typedef errno_t (*sf_connect_in_func)(void *cookie, socket_t so,
    const struct sockaddr *from);

   
                                     
  
                                                                     
                                                                            
                                     
                                                                        
                        
                                                        
                                                                 
                        
                                                                           
                                    
                                                                                  
                                                                                            
                                                              
                                                                       
                                    
   
typedef errno_t (*sf_connect_out_func)(void *cookie, socket_t so,
    const struct sockaddr *to);

   
                              
  
                                                                    
                                       
                                                                        
                        
                                                        
                                                                    
                        
                                                                                 
                                                                                  
                                                                                         
                                                              
                                                                    
   
typedef errno_t (*sf_bind_func)(void *cookie, socket_t so,
    const struct sockaddr *to);

   
                                   
  
                                                                             
                             
                                                                        
                        
                                                        
                                             
                        
                                                                           
                                    
                                                                                  
                                                                                   
                                                                                       
                                                                           
                                    
   
typedef errno_t (*sf_setoption_func)(void *cookie, socket_t so, sockopt_t opt);

   
                                   
  
                                                                             
                             
                                                                        
                        
                                                        
                                             
                        
                                                                           
                                    
                                                                                  
                                                                                   
                                                                                       
                                                                           
                                    
   
typedef errno_t (*sf_getoption_func)(void *cookie, socket_t so, sockopt_t opt);

   
                                
  
                                                                      
                             
                                                                        
                        
                                                        
                        
                                                                               
                                                                                  
                                                                                   
                                                
                                                                           
                                    
   
typedef errno_t (*sf_listen_func)(void *cookie, socket_t so);

   
                               
  
                                                                       
                             
  
                                                                              
                                                                              
                                               
                                                                        
                        
                                                        
                                       
                                                      
                        
                                                                       
                                    
                                                                                  
                                                                                  
                                   
                                                                           
                                    
   
typedef errno_t (*sf_ioctl_func)(void *cookie, socket_t so,
    unsigned long request, const char* argp);

   
                                
  
                                                                        
                                                                        
                                                                         
                                                                       
                                                                         
                                                                        
                                                                   
                                                                                
                                               
                                                           
                                                                           
                                                                             
                        
                                                                               
                                                                             
                                                                                 
                                                                              
                                                                             
                                                                             
                                                              
                                                                             
                                                                        
                                                                               
                                                                          
                                                                        
                                                                    
   
typedef errno_t (*sf_accept_func)(void *cookie, socket_t so_listen, socket_t so,
    const struct sockaddr *local, const struct sockaddr *remote);

   
                            
                                                                      
                                                                
                                                                                
                                                                            
                        
                                                                           
                                                                         
                                                                        
                                                                          
                                   
                                                       
                                                                          
                                              
                                                                         
                                                                           
                                                                         
                                                                           
                      
                                                                            
                      
                                                                
                                          
                                                                  
                                          
                                                                      
                                                                                
                                                                                
                                                                         
                                                                        
                                                                              
                                                                          
                                                                          
                                                                
                                                                            
                                              
                                                                        
                                                 
   
struct sflt_filter {
	sflt_handle                     sf_handle;
	int                             sf_flags;
	char                            *sf_name;

	sf_unregistered_func            sf_unregistered;
	sf_attach_func                  sf_attach;
	sf_detach_func                  sf_detach;

	sf_notify_func                  sf_notify;
	sf_getpeername_func             sf_getpeername;
	sf_getsockname_func             sf_getsockname;
	sf_data_in_func                 sf_data_in;
	sf_data_out_func                sf_data_out;
	sf_connect_in_func              sf_connect_in;
	sf_connect_out_func             sf_connect_out;
	sf_bind_func                    sf_bind;
	sf_setoption_func               sf_setoption;
	sf_getoption_func               sf_getoption;
	sf_listen_func                  sf_listen;
	sf_ioctl_func                   sf_ioctl;
	  
                                                              
                                                              
                                                             
    
	struct sflt_filter_ext {
		unsigned int            sf_ext_len;
		sf_accept_func          sf_ext_accept;
		void                    *sf_ext_rsvd[5];                      
	} sf_ext;
#define sf_len          sf_ext.sf_ext_len
#define sf_accept       sf_ext.sf_ext_accept
};

   
                                
                                                                        
                                                          
                                                         
                                                                             
                                                               
                                                                       
                                                                        
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t sflt_register_internal(const struct sflt_filter *filter,
    int domain, int type, int protocol);

#define sflt_register(filter, domain, type, protocol) \
    sflt_register_internal((filter), (domain), (type), (protocol))
#else
extern errno_t sflt_register(const struct sflt_filter *filter, int domain,
    int type, int protocol)
__NKE_API_DEPRECATED;
#endif                     

   
                                  
                                                                          
                                                                            
                                                                                 
                                    
                                                                        
                                                        
   
extern errno_t sflt_unregister(sflt_handle handle)
__NKE_API_DEPRECATED;

   
                              
                                                                        
                                                                     
                                                                   
                                                                          
                                                        
   
extern errno_t sflt_attach(socket_t socket, sflt_handle handle)
__NKE_API_DEPRECATED;

   
                              
                                                                      
                                                                     
                                                                          
                                                        
   
extern errno_t sflt_detach(socket_t socket, sflt_handle handle)
__NKE_API_DEPRECATED;

                                        
  
                                                              
                             
  
                                      
   

   
                                      
                                                                             
                                              
                                                    
                                                                    
                                                                                 
                                                                          
                          
                                                         
                                                   
                                                        
                                                                        
                                                                            
                      
   
extern errno_t sock_inject_data_in(socket_t so, const struct sockaddr *from,
    mbuf_t data, mbuf_t control, sflt_data_flag_t flags)
__NKE_API_DEPRECATED;

   
                                       
                                                                             
                                          
                                                    
                                                                            
                                                                                
                                                               
                                                         
                                                   
                                                        
                                                                             
                                                                    
   
extern errno_t sock_inject_data_out(socket_t so, const struct sockaddr *to,
    mbuf_t data, mbuf_t control, sflt_data_flag_t flags)
__NKE_API_DEPRECATED;


  
                      
   

enum {
	sockopt_get     = 1,
	sockopt_set     = 2
};
typedef u_int8_t sockopt_dir;

   
                                    
                                                                         
                      
                                       
                                              
   
extern sockopt_dir sockopt_direction(sockopt_t sopt)
__NKE_API_DEPRECATED;

   
                                
                                                                          
                                       
                                                              
   
extern int sockopt_level(sockopt_t sopt)
__NKE_API_DEPRECATED;

   
                               
                                                                        
                                       
                                                             
   
extern int sockopt_name(sockopt_t sopt)
__NKE_API_DEPRECATED;

   
                                  
                                                                  
                                       
                                                   
   
extern size_t sockopt_valsize(sockopt_t sopt)
__NKE_API_DEPRECATED;

   
                                 
                                                                        
                                       
                                                                    
                                                   
                                                     
   
extern errno_t sockopt_copyin(sockopt_t sopt, void *data, size_t length)
__NKE_API_DEPRECATED;

   
                                  
                                                                      
                                       
                                                                     
                                                   
                                                     
   
extern errno_t sockopt_copyout(sockopt_t sopt, void *data, size_t length)
__NKE_API_DEPRECATED;

__END_DECLS
#undef __NKE_API_DEPRECATED
#endif                           
