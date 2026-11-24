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
   
                           
                                                                             
                                                                   
  
                                                                           
                                                                       
                                                                        
                                                                             
                                                                          
                                                               
  
                                                                          
                                                                          
                                                                            
                                                        
   

#ifndef __KPI_MBUF__
#define __KPI_MBUF__
#include <sys/kernel_types.h>
#include <mach/vm_types.h>

#ifndef PRIVATE
#include <Availability.h>
#define __NKE_API_DEPRECATED __API_DEPRECATED("Network Kernel Extension KPI is deprecated", macos(10.4, 10.15.4))
#else
#define __NKE_API_DEPRECATED
#endif              

#ifdef KERNEL_PRIVATE
#include <mach/kern_return.h>
#endif                     

   
                           
                                                                             
                                         
                                                                  
                                                                       
                                                                       
                                                                  
                                                                       
                                        
                                                                       
                                         
                                                                            
                        
                                                                              
                                                                            
                                                                       
                                                                                 
                                                                             
                                                       
   
enum {
	MBUF_EXT        = 0x0001,                                            
	MBUF_PKTHDR     = 0x0002,                            
	MBUF_EOR        = 0x0004,                          
	MBUF_LOOP       = 0x0040,                                  

	MBUF_BCAST      = 0x0100,                                                  
	MBUF_MCAST      = 0x0200,                                                  
	MBUF_FRAG       = 0x0400,                                                    
	MBUF_FIRSTFRAG  = 0x0800,                                     
	MBUF_LASTFRAG   = 0x1000,                                    
	MBUF_PROMISC    = 0x2000,                                  
	MBUF_HASFCS     = 0x4000                            
};
typedef u_int32_t mbuf_flags_t;

   
                          
                                  
                                                                             
                                                                            
                                                                                 
                               
                                                                 
                                                                                 
                                                                               
                       
                                                                          
                      
                                                                          
                                           
                                                   
                                                      
                                                      
                                                     
                                                                
                                                                         
                      
                                                             
                                                
                                                    
                                                                       
                          
                                                    
   
enum {
	MBUF_TYPE_FREE          = 0,                                
	MBUF_TYPE_DATA          = 1,                                   
	MBUF_TYPE_HEADER        = 2,                       
	MBUF_TYPE_SOCKET        = 3,                          
	MBUF_TYPE_PCB           = 4,                                
	MBUF_TYPE_RTABLE        = 5,                        
	MBUF_TYPE_HTABLE        = 6,                         
	MBUF_TYPE_ATABLE        = 7,                                   
	MBUF_TYPE_SONAME        = 8,                     
	MBUF_TYPE_SOOPTS        = 10,                       
	MBUF_TYPE_FTABLE        = 11,                                   
	MBUF_TYPE_RIGHTS        = 12,                      
	MBUF_TYPE_IFADDR        = 13,                          
	MBUF_TYPE_CONTROL       = 14,                                    
	MBUF_TYPE_OOBDATA       = 15                         
};
typedef u_int32_t mbuf_type_t;

   
                                        
                                                      
                                                                        
                                                                                
                                                                           
                                                                                
                                                                           
                                                   
                                                                          
                                
                                                                            
                                
                                                                            
                                
                                                                            
                                             
                                                                            
                                             
   
enum {
	MBUF_TSO_IPV4           = 0x100000,
	MBUF_TSO_IPV6           = 0x200000
};
typedef u_int32_t mbuf_tso_request_flags_t;

enum {
#ifdef KERNEL_PRIVATE
	MBUF_CSUM_PARTIAL       = 0x1000,                                      
	MBUF_CSUM_REQ_SUM16     = MBUF_CSUM_PARTIAL,
	MBUF_CSUM_REQ_ZERO_INVERT = 0x2000,
#endif                     
	MBUF_CSUM_REQ_IP        = 0x0001,
	MBUF_CSUM_REQ_TCP       = 0x0002,
	MBUF_CSUM_REQ_UDP       = 0x0004,
	MBUF_CSUM_REQ_TCPIPV6   = 0x0020,
	MBUF_CSUM_REQ_UDPIPV6   = 0x0040
};
typedef u_int32_t mbuf_csum_request_flags_t;

   
                                          
                                                      
                                                                        
                                                                                 
                                                                               
                          
                                                                               
                                             
                                                                             
                                                                          
                                                                            
                                                                         
                                                                     
                                                                                
                                                                      
                                                                               
                                                                       
                                                                                 
                                                                               
                                                                                 
                                                              
   
enum {
#ifdef KERNEL_PRIVATE
	MBUF_CSUM_TCP_SUM16     = MBUF_CSUM_PARTIAL,
#endif                     
	MBUF_CSUM_DID_IP        = 0x0100,
	MBUF_CSUM_IP_GOOD       = 0x0200,
	MBUF_CSUM_DID_DATA      = 0x0400,
	MBUF_CSUM_PSEUDO_HDR    = 0x0800
};
typedef u_int32_t mbuf_csum_performed_flags_t;

   
                         
                                                
                                                                    
                                                                               
                                                                 
                                                                         
                                                                        
                                                                             
   
enum {
	MBUF_WAITOK     = 0,                                   
	MBUF_DONTWAIT   = 1                                                          
};
typedef u_int32_t mbuf_how_t;

typedef u_int32_t mbuf_tag_id_t;
typedef u_int16_t mbuf_tag_type_t;

   
                          
                                                            
                                                          
                                                                
                                               
                                                        
                                                        
                                                                           
                                                                         
                                                      
                                                       
                                        
                                                   
                                                                       
                                                                              
                                               
                                                                     
                                                   
                                                        
                                                         
   
struct mbuf_stat {
	u_int32_t       mbufs;                                             
	u_int32_t       clusters;                                             
	u_int32_t       clfree;                            
	u_int32_t       drops;                                          
	u_int32_t       wait;                                       
	u_int32_t       drain;                                                 
	u_short         mtypes[256];                                        
	u_int32_t       mcfail;                                   
	u_int32_t       mpfail;                                    
	u_int32_t       msize;                                 
	u_int32_t       mclbytes;                                      
	u_int32_t       minclsize;                                                    
	u_int32_t       mlen;                                          
	u_int32_t       mhlen;                                               
	u_int32_t       bigclusters;                                
	u_int32_t       bigclfree;                                       
	u_int32_t       bigmclbytes;                                         
};

                                             
#define MBUF_COPYALL    1000000000

__BEGIN_DECLS
                 
   
                            
                                                                         
                                                                               
                                                                             
                                                                               
                                                                             
                                                                            
                                                                               
                                                                            
                                                                              
                                                                             
                                                                             
                                                                               
                                                                              
                              
                                                   
   
extern void *mbuf_data(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                 
                                                                         
                                                                              
                                                                        
                                                                           
                                                                                
                                                                          
                              
                                                               
   
extern void *mbuf_datastart(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                               
                                                                          
                                                                               
                                                                            
                                                                             
                                                                            
                                                                         
                                                                   
                              
                                                    
                                                       
                                                      
   
extern errno_t mbuf_setdata(mbuf_t mbuf, void *data, size_t len)
__NKE_API_DEPRECATED;

   
                                
                                                                             
                                                                                 
                                                                               
                                                       
                              
                                                                          
                               
                                                      
   
extern errno_t mbuf_align_32(mbuf_t mbuf, size_t len)
__NKE_API_DEPRECATED;

   
                                        
                                                                             
                                                                           
                                                                               
                                                                               
                                                                            
                                                                             
                                                                             
                                                                          
                                                                            
                                                                                
                                                              
                                                        
                                                                            
                                                          
   
extern addr64_t mbuf_data_to_physical(void *ptr)
__NKE_API_DEPRECATED;


                

   
                           
                                                                           
                                             
                                          
                              
                                                      
   
extern errno_t mbuf_get(mbuf_how_t how, mbuf_type_t type, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                              
                                                                           
                                                                                 
                                   
                                             
                                          
                              
                                                      
   
extern errno_t mbuf_gethdr(mbuf_how_t how, mbuf_type_t type, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                                     
                                                                                 
                                                                                
                                                                              
                                                                               
                                                                
                                             
                                                                                 
                                                                              
                                                                                
                                                                                
                                                                               
                                                                                 
                                                               
                                                      
                                                                           
                                                                            
                               
                                                    
                                                                            
                                                                 
                             
                                           
                                                     
   
extern errno_t mbuf_attachcluster(mbuf_how_t how, mbuf_type_t type,
    mbuf_t *mbuf, caddr_t extbuf, void (*extfree)(caddr_t, u_int, caddr_t),
    size_t extsize, caddr_t extarg)
__NKE_API_DEPRECATED;

   
                                    
                                                                        
                                                                             
                                                                         
                                                                        
                                                                          
                                                                             
                                                                              
                                                                            
                                             
                                                                            
                                                                           
                                                                             
                                             
                                                                     
                                                                           
                                                                            
                                                                            
                                                                        
                                                                         
                                                   
   
extern errno_t mbuf_alloccluster(mbuf_how_t how, size_t *size, caddr_t *addr)
__NKE_API_DEPRECATED;

   
                                   
                                                                           
                                                                         
                                                                        
                                                                              
                                                
                                                    
   
extern void mbuf_freecluster(caddr_t addr, size_t size)
__NKE_API_DEPRECATED;

#ifdef BSD_KERNEL_PRIVATE
  
                                                                 
                                                   
   
extern errno_t mbuf_ring_cluster_alloc(mbuf_how_t how, mbuf_type_t type,
    mbuf_t *mbuf, void (*extfree)(caddr_t, u_int, caddr_t), size_t *size);
extern int mbuf_ring_cluster_is_active(mbuf_t mbuf);
extern errno_t mbuf_ring_cluster_activate(mbuf_t mbuf);
extern errno_t mbuf_cluster_set_prop(mbuf_t mbuf, u_int32_t oldprop,
    u_int32_t newprop);
extern errno_t mbuf_cluster_get_prop(mbuf_t mbuf, u_int32_t *prop);
#endif                         

   
                                  
                                                                              
                                                                           
                                                                             
                                                                          
                                                   
                                             
                                          
                                                                             
                                                                           
                                                                          
                                                                        
                                                                        
                                                              
                                                                            
                                                                                 
                                                                      
                                              
                                           
                                                     
                                                                           
                                                                              
                                                                               
                                                                             
                                                        
   
extern errno_t mbuf_getcluster(mbuf_how_t how, mbuf_type_t type, size_t size,
    mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                              
                                                                           
                                                                                
                                                                        
                                                                           
                           
                                             
                                          
                                                              
                                                                            
                                                                                 
                                                                      
                                              
   
extern errno_t mbuf_mclget(mbuf_how_t how, mbuf_type_t type, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                                   
                                                                           
                                                                              
                                                                       
                                                                             
                        
                                            
                                                                              
                                                      
                                                                                
                                                                               
                                                                              
                                                                             
                                                                             
                                                                             
                                                                             
                                                                         
                                                                          
                                       
                                                                        
                                                                           
                                                                          
                                                                       
                                                                         
                                                                            
                                                                        
                                    
                                                                             
                                                                    
                                           
                                                     
                                                                           
                                     
   
extern errno_t mbuf_allocpacket(mbuf_how_t how, size_t packetlen,
    unsigned int * maxchunks, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                                        
                                                                         
                                                                          
                                                                             
                                                                         
                                                    
                                                     
                                            
                                                                              
                                                      
                                                                          
                                                                               
                                                                               
                                                                               
                                                                             
                                                                             
                                                                             
                                                                         
                                                                          
                                       
                                                                        
                                                                           
                                                                          
                                                                       
                                                                         
                                                                            
                                                                        
                                    
                                                                             
                                                                    
                                           
                                                     
                                                                           
                                     
   
extern errno_t mbuf_allocpacket_list(unsigned int numpkts, mbuf_how_t how,
    size_t packetlen, unsigned int * maxchunks, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                                 
                                                                             
                                        
                                             
                                                                             
                                                      
   
extern errno_t mbuf_getpacket(mbuf_how_t how, mbuf_t *mbuf)
__NKE_API_DEPRECATED;

   
                            
                                                                      
                                                                  
                                      
                                            
   
extern mbuf_t mbuf_free(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                             
                                                               
                                                         
   
extern void mbuf_freem(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                  
                                                                    
                                                                          
                                                               
                                           
   
extern int mbuf_freem_list(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                    
                                                                          
                                  
                              
                                                                     
   
extern size_t mbuf_leadingspace(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                     
                                                                         
                                  
                              
                                                                       
   
extern size_t mbuf_trailingspace(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

                  

   
                             
                                                                             
                                                                          
                                                                           
                                                                          
                                                              
                                    
                                                                    
                                                    
                                                                 
                                                                
                                                          
   
extern errno_t mbuf_copym(const mbuf_t src, size_t offset, size_t len,
    mbuf_how_t how, mbuf_t *new_mbuf)
__NKE_API_DEPRECATED;

   
                           
                                                                            
                                                                              
                                                                              
                                    
                                    
                                             
                                                                
                                                          
   
extern errno_t mbuf_dup(const mbuf_t src, mbuf_how_t how, mbuf_t *new_mbuf)
__NKE_API_DEPRECATED;

   
                               
                                                                    
                                                                               
                                                                                
                                                                               
                                                                   
                                                                          
                                                                              
                                                                      
                                             
                                                          
   
extern errno_t mbuf_prepend(mbuf_t *mbuf, size_t len, mbuf_how_t how)
__NKE_API_DEPRECATED;

   
                             
                                                              
                                         
                                                                        
                       
                                             
                                                                        
                       
                                                                         
                                                                          
                           
   
extern errno_t mbuf_split(mbuf_t src, size_t offset, mbuf_how_t how,
    mbuf_t *new_mbuf)
__NKE_API_DEPRECATED;

   
                              
                                                                           
                                                                              
                                                                               
                                                 
                                                                            
                                                                      
                                                                            
                                                      
   
extern errno_t mbuf_pullup(mbuf_t *mbuf, size_t len)
__NKE_API_DEPRECATED;

   
                                
                                                                  
                                                                             
                                                                             
                                                                            
                                                                               
                                     
                                                
                                                                          
                                                                                
                                                                              
                          
                                                                         
                                                                             
                    
                                                          
   
extern errno_t mbuf_pulldown(mbuf_t src, size_t *offset, size_t length,
    mbuf_t *location)
__NKE_API_DEPRECATED;

   
                           
                                                                            
                                                                                
                                                                                 
                                   
                                            
                                                                    
   
extern void mbuf_adj(mbuf_t mbuf, int len)
__NKE_API_DEPRECATED;

   
                                 
                                                                       
                                                                             
                                                                             
                                                                           
                                                   
                                        
                                                                   
                                                          
   
extern errno_t mbuf_adjustlen(mbuf_t mbuf, int amount)
__NKE_API_DEPRECATED;

   
                                   
                                                                              
                                                                              
                                                                          
                                                                          
                                                                              
                                                                              
                                            
                                               
                                          
                                                                            
                                                                             
                                                                            
                                                                           
                               
   
extern mbuf_t mbuf_concatenate(mbuf_t dst, mbuf_t src)
__NKE_API_DEPRECATED;

   
                                
                                                                            
                                                                                
                                                                                 
                                                        
                                                                  
                                                   
                                                                         
                        
                                                          
   
extern errno_t mbuf_copydata(const mbuf_t mbuf, size_t offset, size_t length,
    void *out_data)
__NKE_API_DEPRECATED;

   
                                
                                                                
                                                                               
  
                                                                                
                                                                                
                                                                            
                                     
  
                                                                             
                                                                               
                                                                           
                                                                    
                                                                        
                                                                   
                                                                               
                       
                                                                     
                                             
                                                                
   
extern errno_t mbuf_copyback(mbuf_t mbuf, size_t offset, size_t length,
    const void *data, mbuf_how_t how)
__NKE_API_DEPRECATED;

   
                                       
                                                                                 
                                                                               
                                         
                                                       
                                                                         
   
extern int mbuf_mclhasreference(mbuf_t mbuf)
__NKE_API_DEPRECATED;


                 

   
                            
                                                        
                              
                                            
   
extern mbuf_t mbuf_next(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                               
                                                     
                              
                                       
                                                          
   
extern errno_t mbuf_setnext(mbuf_t mbuf, mbuf_t next)
__NKE_API_DEPRECATED;

   
                               
                                                        
                              
                             
   
extern mbuf_t mbuf_nextpkt(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                  
                                                                
                              
                                            
   
extern void mbuf_setnextpkt(mbuf_t mbuf, mbuf_t nextpkt)
__NKE_API_DEPRECATED;

   
                           
                                                          
                              
                            
   
extern size_t mbuf_len(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                              
                                                                          
                                                                         
                              
                                   
   
extern void mbuf_setlen(mbuf_t mbuf, size_t len)
__NKE_API_DEPRECATED;

   
                              
                                                                            
                                                                               
                                                                    
                                   
                              
                                                          
   
extern size_t mbuf_maxlen(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                            
                                           
                              
                          
   
extern mbuf_type_t mbuf_type(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                               
                                           
                              
                                      
                                                          
   
extern errno_t mbuf_settype(mbuf_t mbuf, mbuf_type_t new_type)
__NKE_API_DEPRECATED;

   
                             
                                           
                              
                           
   
extern mbuf_flags_t mbuf_flags(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                
                                               
                              
                                                                           
                                                                            
                                                          
   
extern errno_t mbuf_setflags(mbuf_t mbuf, mbuf_flags_t flags)
__NKE_API_DEPRECATED;

   
                                     
                                                                            
                                                                       
                              
                                                                             
                                                    
                                                                       
                                                          
   
extern errno_t mbuf_setflags_mask(mbuf_t mbuf, mbuf_flags_t flags,
    mbuf_flags_t mask)
__NKE_API_DEPRECATED;

   
                                   
                                                               
                                                                         
                                                                        
                                                          
   
extern errno_t mbuf_copy_pkthdr(mbuf_t dest, const mbuf_t src)
__NKE_API_DEPRECATED;

   
                                  
                                                                         
                                                          
                                                     
   
extern size_t mbuf_pkthdr_len(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                     
                                                                        
                                                           
                                                 
   
extern void mbuf_pkthdr_setlen(mbuf_t mbuf, size_t len)
__NKE_API_DEPRECATED;

#ifdef XNU_KERNEL_PRIVATE
   
                                     
                                                                            
                                                                              
                                                                             
                                                  
                              
                                                          
   
extern size_t mbuf_pkthdr_maxlen(const mbuf_t mbuf);
#endif                         

   
                                        
                                                                           
                                                           
                                                                             
                          
   
extern void mbuf_pkthdr_adjustlen(mbuf_t mbuf, int amount)
__NKE_API_DEPRECATED;

   
                                    
                                                                           
                                                                               
                                                                                 
                                                                               
                                                                              
                                                                                 
                                                                         
                                                           
                                              
   
extern ifnet_t mbuf_pkthdr_rcvif(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                       
                                                                   
                                                           
                                                
                                                          
   
extern errno_t mbuf_pkthdr_setrcvif(mbuf_t mbuf, ifnet_t ifp)
__NKE_API_DEPRECATED;

   
                                     
                                                            
                                                           
                                                
   
extern void *mbuf_pkthdr_header(const mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                        
                                                           
                                                           
                                               
   
extern void mbuf_pkthdr_setheader(mbuf_t mbuf, void *header)
__NKE_API_DEPRECATED;

               

   
                                        
                                                                            
                                                                        
                                                                           
                                                                               
                                                                              
                                                                                
                                                                             
                                                                              
                                             
                                                     
   
extern void mbuf_inbound_modified(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                         
                                                                           
                                                  
  
                                                                                 
                                                                             
                                                                                
                                                                               
                                                                                 
                                                                              
                                                                                
                                                                                
                                                                              
                                                                                
                                                                                 
                                                                                
                                    
  
                                                                             
                         
  
                                                                             
                                                                                
                                                                              
                                                                               
                                                                           
                        
                                                       
                                                                        
                      
                                                                            
                                                                                
                                                           
   
extern void mbuf_outbound_finalize(mbuf_t mbuf, u_int32_t protocol_family,
    size_t protocol_offset)
__NKE_API_DEPRECATED;

   
                                    
                                                                          
                                                                              
                                                                         
                                                    
                                                                
                                                          
   
extern errno_t mbuf_set_vlan_tag(mbuf_t mbuf, u_int16_t vlan)
__NKE_API_DEPRECATED;

   
                                    
                                                                           
                                                                                
                                                                              
                                                                               
                                             
                                                    
                                                                
                                                                          
                                              
   
extern errno_t mbuf_get_vlan_tag(mbuf_t mbuf, u_int16_t *vlan)
__NKE_API_DEPRECATED;

   
                                      
                                                                          
                          
                                                    
                                                          
   
extern errno_t mbuf_clear_vlan_tag(mbuf_t mbuf)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                          
                                                                         
                                                                               
                                                                            
                                                                            
                                                                            
                                                                  
                                                    
                                                                            
                                 
                                                              
                                                          
   
extern errno_t mbuf_set_csum_requested(mbuf_t mbuf,
    mbuf_csum_request_flags_t request, u_int32_t value);
#endif                     

   
                                          
                                                                           
                                                                     
                                                    
                                                                            
                                 
                                                              
                                                          
   
extern errno_t mbuf_get_csum_requested(mbuf_t mbuf,
    mbuf_csum_request_flags_t *request, u_int32_t *value)
__NKE_API_DEPRECATED;

   
                                         
                                                                           
                                                                     
                                                    
                                                                         
                                 
                                          
                                                          
   
extern errno_t mbuf_get_tso_requested(mbuf_t mbuf,
    mbuf_tso_request_flags_t *request, u_int32_t *value)
__NKE_API_DEPRECATED;

   
                                            
                                                                     
                                                    
                                                          
   
extern errno_t mbuf_clear_csum_requested(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                          
                                                                              
                                                                
                                                    
                                                                         
                                
                                                                            
                                                                               
                          
                                                          
   
extern errno_t mbuf_set_csum_performed(mbuf_t mbuf,
    mbuf_csum_performed_flags_t flags, u_int32_t value)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
  
                                          
                                                                           
                                                                 
                                                    
                                                                         
                                
                                                                          
                                                                               
                          
                                                          
   
extern errno_t mbuf_get_csum_performed(mbuf_t mbuf,
    mbuf_csum_performed_flags_t *flags, u_int32_t *value);
#endif                     

   
                                
                                                                              
                                                                             
                                                                               
                                   
                                                       
   
extern u_int32_t mbuf_get_mlen(void)
__NKE_API_DEPRECATED;

   
                                 
                                                                              
                                                                            
                                                       
   
extern u_int32_t mbuf_get_mhlen(void)
__NKE_API_DEPRECATED;

   
                                     
                                                                          
                                                                               
                                        
                                                                           
   
extern u_int32_t mbuf_get_minclsize(void)
__NKE_API_DEPRECATED;

   
                                            
                                                                   
                                                    
                                                          
   
extern errno_t mbuf_clear_csum_performed(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                  
                                                                              
                                                                             
                                                                            
                                                                            
                                                                           
                                                                           
                                                                          
                                           
  
                                                                             
                                                                            
                                                                           
                                                    
                                                                        
                                                                              
                                                                        
                                                                            
                                                                               
                                                                            
                                                                         
                                                                             
                                                                            
                                                          
   
extern errno_t mbuf_inet_cksum(mbuf_t mbuf, int protocol, u_int32_t offset,
    u_int32_t length, u_int16_t *csum)
__NKE_API_DEPRECATED;

   
                                   
                                                                              
                                                                             
                                                                            
                                                                            
                                                                           
                                                                           
                                                                          
                                           
  
                                                                             
                                                                            
                                                                              
                                                    
                                                                        
                                                                              
                                                                        
                                                                            
                                                                               
                                                                            
                                                                         
                                                                             
                                                                            
                                                          
   
extern errno_t mbuf_inet6_cksum(mbuf_t mbuf, int protocol, u_int32_t offset,
    u_int32_t length, u_int16_t *csum)
__NKE_API_DEPRECATED;

               

   
                                   
                                                                             
                                                                              
                                                                                 
                                                                               
                                                
  
                                                                                 
                                                                           
                                                                             
                                                                      
                                                 
                                                                       
                                                                                 
                             
                                                          
   
extern errno_t mbuf_tag_id_find(const char *module_string,
    mbuf_tag_id_t *module_id)
__NKE_API_DEPRECATED;

   
                                    
                                                                           
                                                                              
                                        
  
                                                                    
                                                                                
                                                       
                                                    
                                                                           
                                                                            
                                                 
                                                                         
                                                               
                                                                             
                                                     
                                                                        
                                               
                                                          
   
extern errno_t mbuf_tag_allocate(mbuf_t mbuf, mbuf_tag_id_t module_id,
    mbuf_tag_type_t type, size_t length, mbuf_how_t how, void **data_p)
__NKE_API_DEPRECATED;

   
                                
                                                               
                                                     
                                                                           
                                                        
                                                                        
          
                                                                        
                                               
                                                          
   
extern errno_t mbuf_tag_find(mbuf_t mbuf, mbuf_tag_id_t module_id,
    mbuf_tag_type_t type, size_t *length, void **data_p)
__NKE_API_DEPRECATED;

   
                                
                                                           
                                                       
                                                    
                                                 
   
extern void mbuf_tag_free(mbuf_t mbuf, mbuf_tag_id_t module_id,
    mbuf_tag_type_t type)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE
   
                                  
                                                                           
                                                                              
                                                                          
                                                                              
                                                                               
                                                                              
                                                                              
                                                                              
                                                                            
                                                                            
                              
                                                              
                                                                          
                                                                
                                                                              
                                                                
                                     
                                                                                
                                                                        
                                                                            
                                                          
   
extern errno_t mbuf_add_drvaux(mbuf_t mbuf, mbuf_how_t how,
    u_int32_t family, u_int32_t subfamily, size_t length, void **data_p);

   
                                   
                                                                             
                                                                
                                                                       
                                                                          
                                                                      
                                                                             
                                                                          
                                                                      
                                                                       
                                                                     
                                  
                                                                        
                                              
                                                          
   
extern errno_t mbuf_find_drvaux(mbuf_t mbuf, u_int32_t *family_p,
    u_int32_t *subfamily_p, u_int32_t *length_p, void **data_p);

   
                                  
                                                                        
                                 
                                                                
   
extern void mbuf_del_drvaux(mbuf_t mbuf);
#endif                     

                

   
                             
                                             
                                                      
   
extern void mbuf_stats(struct mbuf_stat *stats)
__NKE_API_DEPRECATED;


   
                                   
                                            
                                                                                 
                                                                                  
                                                        
                                                                       
                                                                                
                                                                                   
   
typedef enum {
#ifdef XNU_KERNEL_PRIVATE
	MBUF_TC_UNSPEC  = -1,                                        
#endif
	MBUF_TC_BE              = 0,
	MBUF_TC_BK              = 1,
	MBUF_TC_VI              = 2,
	MBUF_TC_VO              = 3
#ifdef XNU_KERNEL_PRIVATE
	,
	MBUF_TC_MAX             = 4                                        
#endif
} mbuf_traffic_class_t;

   
                                         
                                                            
                                                          
                                  
   
extern mbuf_traffic_class_t mbuf_get_traffic_class(mbuf_t mbuf)
__NKE_API_DEPRECATED;

   
                                         
                                                             
                                                          
                                    
                                                                
   
extern errno_t mbuf_set_traffic_class(mbuf_t mbuf, mbuf_traffic_class_t tc)
__NKE_API_DEPRECATED;

   
                                                   
                                                                       
                                                     
                                                          
                                                     
   
extern int mbuf_is_traffic_class_privileged(mbuf_t mbuf)
__NKE_API_DEPRECATED;

#ifdef KERNEL_PRIVATE

   
                                                   
                                                                           
                                                        
   
extern u_int32_t mbuf_get_traffic_class_max_count(void);

   
                                               
                                                                                
                                    
                                                
                                                                
   
extern errno_t mbuf_get_traffic_class_index(mbuf_traffic_class_t tc,
    u_int32_t *index);

   
                               
                                            
                                                                     
                                                                        
                                       
                                                                           
                                                                            
                            
                                                                                
                                                                             
                                                                     
                                                                            
                                                                            
                                                                               
                             
                                                                             
                                                                            
                            
                                                                            
                                                                           
                                                
                                                                              
                                                                           
                                                       
                                                                            
                                                                           
                                                
                                                                           
                                                                              
                                                                            
                                          
                                                                        
                                                                              
                                                                               
                                                 
                                                                               
                                                                               
                                                                          
                            
                                                                              
                                                                                
   
typedef enum {
#ifdef XNU_KERNEL_PRIVATE
	MBUF_SC_UNSPEC          = -1,                                        
#endif
	MBUF_SC_BK_SYS          = 0x00080090,                     
	MBUF_SC_BK              = 0x00100080,

	MBUF_SC_BE              = 0x00000000,
	MBUF_SC_RD              = 0x00180010,
	MBUF_SC_OAM             = 0x00200020,

	MBUF_SC_AV              = 0x00280120,
	MBUF_SC_RV              = 0x00300110,
	MBUF_SC_VI              = 0x00380100,
	MBUF_SC_SIG             = 0x00380130,

	MBUF_SC_VO              = 0x00400180,
	MBUF_SC_CTL             = 0x00480190,                      
} mbuf_svc_class_t;

   
                                                   
                                                                            
                                                         
   
extern u_int32_t mbuf_get_service_class_max_count(void);

   
                                               
                                                                                
                                    
                                                
                                                                
   
extern errno_t mbuf_get_service_class_index(mbuf_svc_class_t sc,
    u_int32_t *index);

   
                                         
                                                            
                                                          
                                  
   
extern mbuf_svc_class_t mbuf_get_service_class(mbuf_t mbuf);

   
                                          
                                                             
                                                          
                                    
                                                                
   
extern errno_t mbuf_set_service_class(mbuf_t mbuf, mbuf_svc_class_t sc);

   
                                                   
                                                                       
                                                     
                                                          
                                                     
   
extern int mbuf_is_service_class_privileged(mbuf_t mbuf);

   
                                      
                                                                           
                                               
                                                                         
                                                                           
                             
                                                                             
                                                                             
                                         
   
enum {
	MBUF_PKTAUXF_INET_RESOLVE_RTR   = 0x0004,
	MBUF_PKTAUXF_INET6_RESOLVE_RTR  = 0x0008,
};
typedef u_int32_t mbuf_pkthdr_aux_flags_t;

   
                                        
                                                             
                                                           
                                                                       
                                                          
   
extern errno_t mbuf_pkthdr_aux_flags(mbuf_t mbuf,
    mbuf_pkthdr_aux_flags_t *paux_flags);

   
                                          
                                                                            
                                                                       
                                                                        
                                                                             
                         
                                                                            
                                     
   
extern errno_t mbuf_get_driver_scratch(mbuf_t m, u_int8_t **area,
    size_t *area_ln);

   
                                             
                                                                          
                                                                          
                                                                      
                           
                                                       
                                                                      
                                         
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_unsent_data_bytes(const mbuf_t m,
    u_int32_t *unsent_data);

typedef struct {
	int32_t buf_interface;                                
	int32_t buf_sndbuf;                                    
} mbuf_buffer_status_t;

   
                                         
                                                                          
                                                                          
                                                                      
                           
                                                       
                                                                        
                                         
                                                                          
                                                                   
                                        
   
extern errno_t mbuf_get_buffer_status(const mbuf_t m,
    mbuf_buffer_status_t *buf_status);

   
                                    
                                                                           
                                                                           
                               
                                                       
                                                                           
                                                                    
                                          
                                                                        
                                                                            
                                                      
   
extern errno_t mbuf_pkt_new_flow(const mbuf_t m, u_int32_t *retval);

   
                                
                                                                        
                                                                   
                                           
                                                       
                                                                         
                                                                      
                             
                                                                        
                                                                     
                                                             
   
extern errno_t mbuf_last_pkt(const mbuf_t m, u_int32_t *retval);

#endif                     

#ifdef XNU_KERNEL_PRIVATE
   
                                    
                                                                      
                              
                                                    
   
extern size_t mbuf_pkt_list_len(const mbuf_t mbuf);

   
                                       
                                                                            
                                                                                     
                                                                            
                                                  
                              
                                                          
   
extern size_t mbuf_pkt_list_maxlen(const mbuf_t mbuf);
#endif                         

#ifdef KERNEL_PRIVATE
   
                                     
                                                           
                                                      
                                                                            
                    
                                                                       
                                                                             
                                                              
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_timestamp(mbuf_t mbuf, u_int64_t *ts, boolean_t *valid);

   
                                     
                                                     
                                                      
                                                                             
                       
                                                                              
                                                                   
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_set_timestamp(mbuf_t mbuf, u_int64_t ts, boolean_t valid);

   
                                    
                                                                        
                                      
                                                                 
                                               
                                                                            
                                           
                                                                               
                                                          
                                                               
                                                                         
                                                         
   

typedef void (*mbuf_tx_compl_func)(uintptr_t pktid, ifnet_t ifp, u_int64_t ts,
    uintptr_t tx_compl_arg, uintptr_t tx_compl_data, kern_return_t tx_compl_val);

   
                                                  
                                                                          
                                                                          
                                
                                                                     
                                                                             
                                                                              
                                                                         
                                                            
   
extern errno_t mbuf_register_tx_compl_callback(
	mbuf_tx_compl_func callback);

   
                                                    
                                                                            
                                                                          
                                
                                                                       
                                                                             
                                                                              
                                                        
   
extern errno_t mbuf_unregister_tx_compl_callback(
	mbuf_tx_compl_func callback);

   
                                               
                                                                                
                                                          
                                                      
                                                                           
                                                   
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_timestamp_requested(mbuf_t mbuf, boolean_t *requested);

   
                                               
                                                                            
                                                                          
                                                
                                                      
                                                                              
                                                                               
                                        
                                                                            
                                                                             
                                                                  
   
extern errno_t mbuf_set_timestamp_requested(mbuf_t mbuf,
    uintptr_t *pktid, mbuf_tx_compl_func callback);

   
                                  
                                                            
                                                      
                                                                              
                              
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_status(mbuf_t mbuf, kern_return_t *status);

   
                                  
                                                                          
                        
                                                      
                                                          
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_set_status(mbuf_t mbuf, kern_return_t status);

   
                                         
                                                            
                                                   
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_tx_compl_data(mbuf_t m, uintptr_t *arg,
    uintptr_t *data);

   
                                         
                                                            
                                                   
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_set_tx_compl_data(mbuf_t m, uintptr_t arg,
    uintptr_t data);

   
                                  
                                                         
                                                      
                                                 
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_flowid(mbuf_t mbuf, u_int16_t *flowid);

   
                                  
                                                    
                                                      
                                             
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_set_flowid(mbuf_t mbuf, u_int16_t flowid);

   
                                          
                                                      
                                                      
                                                                    
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_get_keepalive_flag(mbuf_t mbuf, boolean_t *is_keepalive);

   
                                          
                                                             
                                                      
                                               
                                                                      
                                                                        
                                    
   
extern errno_t mbuf_set_keepalive_flag(mbuf_t mbuf, boolean_t is_keepalive);

#endif                     

                          

#define IF_ENQUEUE_MBUF(ifq, m) {                                       \
	mbuf_setnextpkt((m), 0);                                        \
	if ((ifq)->ifq_tail == 0)                                       \
	        (ifq)->ifq_head = (m);                                  \
	else                                                            \
	        mbuf_setnextpkt((mbuf_t)(ifq)->ifq_tail, (m));          \
	(ifq)->ifq_tail = (m);                                          \
	(ifq)->ifq_len++;                                               \
}

#define IF_PREPEND_MBUF(ifq, m) {                                       \
	mbuf_setnextpkt((m), (ifq)->ifq_head);                          \
	if ((ifq)->ifq_tail == 0)                                       \
	        (ifq)->ifq_tail = (m);                                  \
	(ifq)->ifq_head = (m);                                          \
	(ifq)->ifq_len++;                                               \
}

#define IF_DEQUEUE_MBUF(ifq, m) {                                       \
	(m) = (ifq)->ifq_head;                                          \
	if (m) {                                                        \
	        if (((ifq)->ifq_head = mbuf_nextpkt((m))) == 0)         \
	                (ifq)->ifq_tail = 0;                            \
	        mbuf_setnextpkt((m), 0);                                \
	        (ifq)->ifq_len--;                                       \
	}                                                               \
}

__END_DECLS
#undef __NKE_API_DEPRECATED
#endif                   
