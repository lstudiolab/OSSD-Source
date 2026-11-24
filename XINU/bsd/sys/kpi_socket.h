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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
   
                             
                                                                             
                                                                      
                                                                             
                                                                        
                                                                          
                                                         
   
#ifndef __KPI_SOCKET__
#define __KPI_SOCKET__

#include <sys/types.h>
#include <sys/kernel_types.h>
#include <sys/socket.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15))
#else
#define __NKE_API_DEPRECATED
#endif              

__BEGIN_DECLS

struct timeval;

   
                             
  
                                                                           
                                                                                 
                                                                                
                                                                           
  
                                                                            
                                                                         
  
                                                     
                                                                                 
                                                        
                                                             
                                                   
                                                               
  
                                                          
                                                                        
                                                                  
   
typedef void (*sock_upcall)(socket_t so, void *cookie, int waitf);

#ifdef KERNEL_PRIVATE
   
                               
  
                                                                             
                                                                                 
                                                                                
                                                   
                                                          
                                                                        
                                                                      
   
typedef void (*sock_evupcall)(socket_t so, void *cookie, long event);
#endif                     

   
                              
                                                                           
                                                                                 
                                                                     
                                                                             
                                                                         
                                                         
                                               
                                                                 
                                                                                 
                                                                              
                                                                                 
                                     
                                                                       
                                                        
                                                                
                                                                         
                                                    
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t sock_accept_internal(socket_t so, struct sockaddr *from, int fromlen,
    int flags, sock_upcall callback, void *cookie, socket_t *new_so);

#define sock_accept(so, from, fromlen, flags, callback, cookie, new_so) \
	sock_accept_internal((so), (from), (fromlen), (flags), (callback), \
	(cookie), (new_so))
#else
extern errno_t sock_accept(socket_t so, struct sockaddr *from, int fromlen,
    int flags, sock_upcall callback, void *cookie, socket_t *new_so)
__NKE_API_DEPRECATED;
#endif                     

   
                            
                                                                           
                                      
                                          
                                                                   
                                                        
   
extern errno_t sock_bind(socket_t so, const struct sockaddr *to)
__NKE_API_DEPRECATED;

   
                               
                                                                     
                                               
                                            
                                                                   
                                                                             
                                                                                
                                                                           
                                                                           
                           
                                                                          
                                                              
   
extern errno_t sock_connect(socket_t so, const struct sockaddr *to, int flags)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                                                          
                                                                           
                                                                           
                                               
  
                                                                        
                     
  
                                   
                                                                 
                                              
                                              
                                                                            
                                                                           
                           
   
extern errno_t sock_connectwait(socket_t so, const struct timeval *tv);
#endif                     

   
                                   
                                                                            
                                     
                              
                                                   
                                                                
                                                        
   
extern errno_t sock_getpeername(socket_t so, struct sockaddr *peername,
    int peernamelen)
__NKE_API_DEPRECATED;

   
                                   
                                                                        
                              
                              
                                                    
                                                                  
                                                        
   
extern errno_t sock_getsockname(socket_t so, struct sockaddr *sockname,
    int socknamelen)
__NKE_API_DEPRECATED;

   
                                  
                                                                       
                              
                                                 
                                        
                                        
                                                                       
                                                        
   
extern errno_t sock_getsockopt(socket_t so, int level, int optname,
    void *optval, int *optlen)
__NKE_API_DEPRECATED;

   
                             
                                                                                
                              
                                       
                                  
                                                        
   
extern errno_t sock_ioctl(socket_t so, unsigned long request, void *argp)
__NKE_API_DEPRECATED;

   
                                  
                                                                  
                              
                                                 
                                        
                                        
                                            
                                                        
   
extern errno_t sock_setsockopt(socket_t so, int level, int optname,
    const void *optval, int optlen)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                                                         
                                                                              
  
                                                                        
                     
  
                                    
                                                              
                              
                                        
                                            
                                                        
   
extern errno_t sock_settclassopt(socket_t so, const void* optval, size_t optlen);

  
                                                                         
                         
  
                                                                             
  
                                   
                                                              
                              
                                        
                                                                       
                                                        
   
extern errno_t sock_gettclassopt(socket_t so, void* optval, size_t* optlen);

#ifdef XNU_KERNEL_PRIVATE
extern void socket_set_traffic_mgt_flags_locked(socket_t so, u_int8_t flags);
extern void socket_clear_traffic_mgt_flags_locked(socket_t so, u_int8_t flags);
#endif                         
#ifdef BSD_KERNEL_PRIVATE
extern void socket_set_traffic_mgt_flags(socket_t so, u_int8_t flags);
extern void socket_clear_traffic_mgt_flags(socket_t so, u_int8_t flags);
extern errno_t socket_defunct(struct proc *, socket_t so, int);
extern errno_t sock_receive_internal(socket_t, struct msghdr *, mbuf_t *,
    int, size_t *);
#endif                         
#endif                     

   
                              
                                                                             
                                                 
                              
                                                                               
                                                        
   
extern errno_t sock_listen(socket_t so, int backlog)
__NKE_API_DEPRECATED;

   
                               
                                                                             
                                                                      
                              
                                                                       
                                          
                                                                          
                                  
                                                                        
                                                                            
   
extern errno_t sock_receive(socket_t so, struct msghdr *msg, int flags,
    size_t *recvdlen)
__NKE_API_DEPRECATED;

   
                                   
                                                                        
                                                                                 
                                                           
                              
                                                                           
                                                 
                                                                           
                                                                               
                                                                               
                                          
                                                                             
                                                                           
                                                                    
                                                                        
                                                                            
   
extern errno_t sock_receivembuf(socket_t so, struct msghdr *msg, mbuf_t *data,
    int flags, size_t *recvlen)
__NKE_API_DEPRECATED;

   
                            
                                                                          
                                                                  
                              
                                                                       
                                                           
                                          
                                                 
                                                                        
                                                                            
   
extern errno_t sock_send(socket_t so, const struct msghdr *msg, int flags,
    size_t *sentlen)
__NKE_API_DEPRECATED;

   
                                
                                                                           
                                                                       
                              
                                                                       
                                                     
                                                    
                                          
                                                 
                                                                        
                                                                            
                                                                                 
   
extern errno_t sock_sendmbuf(socket_t so, const struct msghdr *msg, mbuf_t data,
    int flags, size_t *sentlen)
__NKE_API_DEPRECATED;

   
                                
                                                                         
                                                       
                              
                                               
                                         
                                           
                                                        
   
extern errno_t sock_shutdown(socket_t so, int how)
__NKE_API_DEPRECATED;

   
                              
                                                                          
                                                                              
                                                 
                                                           
                                                                       
                                             
                                                                       
                                                        
                                                                
                                                                   
                                                        
   
#ifdef KERNEL_PRIVATE
extern errno_t sock_socket_internal(int domain, int type, int protocol,
    sock_upcall callback, void *cookie, socket_t *new_so);

#define sock_socket(domain, type, protocol, callback, cookie, new_so) \
	sock_socket_internal((domain), (type), (protocol), \
	(callback), (cookie), (new_so))
#else
extern errno_t sock_socket(int domain, int type, int protocol,
    sock_upcall callback, void *cookie, socket_t *new_so)
__NKE_API_DEPRECATED;
#endif                     

   
                             
                                      
                                                                         
                                                                                 
                                                                             
                                                                
   
extern void sock_close(socket_t so)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                              
                                                     
                                                                        
                                                                           
                                                                           
                                                                              
                                                            
                                           
   
extern void sock_retain(socket_t so);

  
                               
                                                                           
                                           
                                                                           
                                                                            
                                                                                 
   
extern void sock_release(socket_t so);
#endif                     

   
                               
                                                                     
                                                         
                                                                  
                                                                          
                                                        
   
extern errno_t sock_setpriv(socket_t so, int on)
__NKE_API_DEPRECATED;

   
                                   
                                                                    
                                       
                                                                      
   
extern int sock_isconnected(socket_t so)
__NKE_API_DEPRECATED;

   
                                     
                                                                          
                                                                              
                                                                             
  
                                                                          
                                                                                
                                                                         
                                                                  
   
extern int sock_isnonblocking(socket_t so)
__NKE_API_DEPRECATED;

   
                               
                                                                             
                                                                              
                                                                          
                           
                                       
                                                                            
                                                                                 
                                                          
                                                        
   
extern errno_t sock_gettype(socket_t so, int *domain, int *type, int *protocol)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                   
                                                                            
                                                  
                                        
                                                                            
                                                        
   
extern errno_t sock_nointerrupt(socket_t so, int on);

  
                                   
                                                                             
                                                                               
                                                                            
                                                                            
                                                                              
                                                                              
                                                                           
                                                                             
                                                                         
                                                                          
                                                                               
                                                                             
                                                              
                                           
                                                                                
                                                                           
                               
   
extern socket_t sock_getlistener(socket_t so);

  
                               
                                                                       
                                                                              
                                                                            
                                                                      
                                                     
                              
                                                                     
                                                                            
                                                        
   
extern errno_t sock_getaddr(socket_t so, struct sockaddr **psockname,
    int peername);

  
                                
                                                                        
                                                     
   
extern void sock_freeaddr(struct sockaddr *sockname);

  
                                 
                                                                         
                                                                         
                                                                     
                                                                               
                                                                         
                                                                            
                                                                            
                                   
                                
                                              
                                                                
   
extern errno_t sock_setupcall(socket_t sock, sock_upcall callback,
    void *context);

  
                                  
                                                                         
                                                                         
                                                                     
                                                                               
                                                                         
                                                                            
                                                                            
                                   
                                
                                                        
                                                                          
                                                          
                                                                            
   
extern errno_t sock_setupcalls(socket_t sock, sock_upcall read_callback,
    void *read_context, sock_upcall write_callback, void *write_context);

  
                                         
                                                          
                                                                            
                                                                            
                                             
   
extern void sock_setupcalls_locked(socket_t sock,
    sock_upcall rcallback, void *rcontext,
    sock_upcall wcallback, void *wcontext, int locked);

  
                                   
                                                                         
                                                                         
                                                                     
                                                                               
                                
                                                          
                                                                            
                                                                            
                                                    
   
extern errno_t sock_catchevents(socket_t sock, sock_evupcall event_callback,
    void *event_context, long event_mask);

extern void sock_catchevents_locked(socket_t sock, sock_evupcall ecallback,
    void *econtext, long emask);


  
                                
                                                                            
                                        
                                
                                                    
   
extern int sock_iskernel(socket_t);
#endif                     

__END_DECLS
#undef __NKE_API_DEPRECATED
#endif                     
