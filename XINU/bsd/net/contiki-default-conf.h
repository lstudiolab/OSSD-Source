/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CONTIKI_DEFAULT_CONF_H
#define CONTIKI_DEFAULT_CONF_H

                                                                               
                         
  
                                                                     
                                              
   

                                                                   
                                                                  
#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO nullradio_driver
                                               
#endif                          

                                                                    
                                                               
                            
#ifndef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER framer_nullmac
                                                
#endif                           

                                                                      
                                                                       
                                                                 
                                                               
                                  
#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC   nullrdc_driver
                                                   
#endif                        

                                                              
                                                      
                                                                     
                                         
#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC   nullmac_driver
                                             
#endif                        

                                                                   
#ifndef NETSTACK_CONF_LLSEC
#define NETSTACK_CONF_LLSEC nullsec_driver
#endif                          

                                                                
                                                               
                                                                     
#ifndef LLSEC802154_CONF_SECURITY_LEVEL
#define LLSEC802154_CONF_SECURITY_LEVEL 0
#endif                                      

                                                                      
                                                                   
                                 
#ifndef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK rime_driver
                                                     
#endif                            

                                                                   
                                                                      
                                                                     
                                                                     
                   
#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif                                           

                                                                               
                              
  
                                                                 
                                      
   

                                                                 
                                                            
                                                                       
                                                            
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 8
#endif                        
                                                                               
                               
  
                                                                
                                                 
   

                                                                                
                                         
#ifndef NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_WITH_IPV6 0
#endif                              

                                                                    
                                                                      
                                                    
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 128
#endif                           

                                                                   
                                                      
#ifndef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER 1
#endif                      

                                                            
               
#ifndef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL 1
#endif                        

                                                                       
                                   
#ifndef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 20
#endif                          

                                                              
                                                                 
#ifndef UIP_CONF_UDP
#define UIP_CONF_UDP 1
#endif                   

                                                           
                                    
#ifndef UIP_CONF_MAX_CONNECTIONS
#define UIP_CONF_MAX_CONNECTIONS 8
#endif                               

                                                              
                                       
#ifndef UIP_CONF_TCP
#define UIP_CONF_TCP 1
#endif                   

                                                           
                                    
#ifndef UIP_CONF_MAX_CONNECTIONS
#define UIP_CONF_MAX_CONNECTIONS 8
#endif                               


                                                                    
                                                                  
                                                              
#ifndef UIP_CONF_TCP_SPLIT
#define UIP_CONF_TCP_SPLIT 0
#endif                         

                                                                         
                                             
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 8
#endif                                   

                                                                          
                                                                               
#ifndef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 1
#endif                           

                                                                               
                                 
  
                                                                      
                                                                     
                                                               
   

                                                                     
                                                            
                                                        
                         
#ifndef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS 4
#endif                                            

                                                                    
                                                   
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#endif                           

                                                                    
                                                                    
                                                                   
                                                                  
#ifndef SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD (127 - 2)
#endif                                      

                                                                   
                                                                      
                                                                     
#ifndef SICSLOWPAN_CONF_COMPRESSION_THRESHOLD
#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 0
                                                      
#endif                                            

                                                                 
                                                                    
#ifndef SICSLOWPAN_CONF_COMPRESSION
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_HC06
#endif                                  

                                                                               
                                    
  
                                                          
   

                                                                  
                                                                      
                                                                     
                                                
#ifndef CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION
#define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 0
#endif                                              


#endif                             
