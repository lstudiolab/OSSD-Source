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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sockio.h	8.1 (Berkeley) 3/28/94
 */

#ifndef _SYS_SOCKIO_H_
#define _SYS_SOCKIO_H_

#include <sys/appleapiopts.h>

#include <sys/ioccom.h>

                     
#define SIOCSHIWAT       _IOW('s',  0, int)                                     
#define SIOCGHIWAT       _IOR('s',  1, int)                                     
#define SIOCSLOWAT       _IOW('s',  2, int)                                    
#define SIOCGLOWAT       _IOR('s',  3, int)                                    
#define SIOCATMARK       _IOR('s',  7, int)                               
#define SIOCSPGRP        _IOW('s',  8, int)                                    
#define SIOCGPGRP        _IOR('s',  9, int)                                    

  
                                                                           
   
#define SIOCSIFADDR     _IOW('i', 12, struct ifreq)                            
#ifdef KERNEL_PRIVATE
#define OSIOCGIFADDR    _IOWR('i', 13, struct ifreq)                    
#endif                     
#define SIOCSIFDSTADDR   _IOW('i', 14, struct ifreq)                         
#ifdef KERNEL_PRIVATE
#define OSIOCGIFDSTADDR _IOWR('i', 15, struct ifreq)                    
#endif                     
#define SIOCSIFFLAGS     _IOW('i', 16, struct ifreq)                         
#define SIOCGIFFLAGS    _IOWR('i', 17, struct ifreq)                         
#ifdef KERNEL_PRIVATE
#define OSIOCGIFBRDADDR _IOWR('i', 18, struct ifreq)                    
#endif                     
#define SIOCSIFBRDADDR   _IOW('i', 19, struct ifreq)                            
#ifdef KERNEL_PRIVATE
#define OSIOCGIFCONF    _IOWR('i', 20, struct ifconf)                   
#define OSIOCGIFCONF32  _IOWR('i', 20, struct ifconf32)                 
#define OSIOCGIFCONF64  _IOWR('i', 20, struct ifconf64)                 
#define OSIOCGIFNETMASK _IOWR('i', 21, struct ifreq)                    
#endif                     
#define SIOCSIFNETMASK   _IOW('i', 22, struct ifreq)                           
#define SIOCGIFMETRIC   _IOWR('i', 23, struct ifreq)                       
#define SIOCSIFMETRIC   _IOW('i', 24, struct ifreq)                        
#define SIOCDIFADDR     _IOW('i', 25, struct ifreq)                         
#define SIOCAIFADDR     _IOW('i', 26, struct ifaliasreq)                      

#define SIOCGIFADDR     _IOWR('i', 33, struct ifreq)                           
#define SIOCGIFDSTADDR  _IOWR('i', 34, struct ifreq)                         
#define SIOCGIFBRDADDR  _IOWR('i', 35, struct ifreq)                            
#if !defined(KERNEL) || defined(KERNEL_PRIVATE)
#define SIOCGIFCONF     _IOWR('i', 36, struct ifconf)                       
#endif                                
#ifdef KERNEL_PRIVATE
#define SIOCGIFCONF32   _IOWR('i', 36, struct ifconf32)                     
#define SIOCGIFCONF64   _IOWR('i', 36, struct ifconf64)                     
#endif                     
#define SIOCGIFNETMASK  _IOWR('i', 37, struct ifreq)                           
#define SIOCAUTOADDR    _IOWR('i', 38, struct ifreq)                          
#define SIOCAUTONETMASK _IOW('i', 39, struct ifreq)                           
#define SIOCARPIPLL             _IOWR('i', 40, struct ifreq)                                

#define SIOCADDMULTI     _IOW('i', 49, struct ifreq)                         
#define SIOCDELMULTI     _IOW('i', 50, struct ifreq)                         
#define SIOCGIFMTU      _IOWR('i', 51, struct ifreq)                    
#define SIOCSIFMTU       _IOW('i', 52, struct ifreq)                    
#define SIOCGIFPHYS     _IOWR('i', 53, struct ifreq)                     
#define SIOCSIFPHYS      _IOW('i', 54, struct ifreq)                     
#define SIOCSIFMEDIA    _IOWR('i', 55, struct ifreq)                       

  
                                                                           
                                                                       
   
#define SIOCGIFMEDIA    _IOWR('i', 56, struct ifmediareq)                                
#ifdef KERNEL_PRIVATE
#define SIOCGIFMEDIA32  _IOWR('i', 56, struct ifmediareq32)                                        
#define SIOCGIFMEDIA64  _IOWR('i', 56, struct ifmediareq64)                                        
#endif                     

#define SIOCSIFGENERIC   _IOW('i', 57, struct ifreq)                           
#define SIOCGIFGENERIC  _IOWR('i', 58, struct ifreq)                           
#define SIOCRSLVMULTI   _IOWR('i', 59, struct rslvmulti_req)

#define SIOCSIFLLADDR   _IOW('i', 60, struct ifreq)                              
#define SIOCGIFSTATUS   _IOWR('i', 61, struct ifstat)                      
#define SIOCSIFPHYADDR   _IOW('i', 62, struct ifaliasreq)                     
#define SIOCGIFPSRCADDR _IOWR('i', 63, struct ifreq)                           
#define SIOCGIFPDSTADDR _IOWR('i', 64, struct ifreq)                           
#define SIOCDIFPHYADDR   _IOW('i', 65, struct ifreq)                          

#define SIOCGIFDEVMTU   _IOWR('i', 68, struct ifreq)                         
#define SIOCSIFALTMTU    _IOW('i', 69, struct ifreq)                              
#define SIOCGIFALTMTU   _IOWR('i', 72, struct ifreq)                              
#define SIOCSIFBOND      _IOW('i', 70, struct ifreq)                            
#define SIOCGIFBOND     _IOWR('i', 71, struct ifreq)                            

  
                                                                             
                                                                     
  
                                                                    
                                                           
   
#define SIOCGIFXMEDIA   _IOWR('i', 72, struct ifmediareq)                             
#ifdef KERNEL_PRIVATE
#define SIOCGIFXMEDIA32 _IOWR('i', 72, struct ifmediareq32)                             
#define SIOCGIFXMEDIA64 _IOWR('i', 72, struct ifmediareq64)                                      
#endif                     

#ifdef PRIVATE
  
                                                                            
   
#define SIOCPROTOATTACH _IOWR('i', 80, struct ifreq)                                   
#define SIOCPROTODETACH _IOWR('i', 81, struct ifreq)                                     
#endif              

#define SIOCSIFCAP       _IOW('i', 90, struct ifreq)                         
#define SIOCGIFCAP      _IOWR('i', 91, struct ifreq)                         

#define SIOCIFCREATE    _IOWR('i', 120, struct ifreq)                        
#define SIOCIFDESTROY    _IOW('i', 121, struct ifreq)                         
#define SIOCIFCREATE2   _IOWR('i', 122, struct ifreq)                                  

#define SIOCSDRVSPEC    _IOW('i', 123, struct ifdrv)                          
                                                                         
#define SIOCGDRVSPEC    _IOWR('i', 123, struct ifdrv)                         
                                                                         
#ifdef KERNEL_PRIVATE
#define SIOCSDRVSPEC32    _IOW('i', 123, struct ifdrv32)                          
                                                                         
#define SIOCGDRVSPEC32    _IOWR('i', 123, struct ifdrv32)                         
                                                                         
#define SIOCSDRVSPEC64    _IOW('i', 123, struct ifdrv64)                          
                                                                         
#define SIOCGDRVSPEC64    _IOWR('i', 123, struct ifdrv64)                         
                                                                         

#endif                     
#define SIOCSIFVLAN      _IOW('i', 126, struct ifreq)                        
#define SIOCGIFVLAN     _IOWR('i', 127, struct ifreq)                        
#define SIOCSETVLAN     SIOCSIFVLAN
#define SIOCGETVLAN     SIOCGIFVLAN
#ifdef KERNEL_PRIVATE
#define SIOCSIFDEVMTU    SIOCSIFALTMTU                                  
#endif                     

#if !defined(KERNEL) || defined(KERNEL_PRIVATE)
#define SIOCIFGCLONERS  _IOWR('i', 129, struct if_clonereq)                  
#endif                                
#ifdef KERNEL_PRIVATE
#define SIOCIFGCLONERS32 _IOWR('i', 129, struct if_clonereq32)                  
#define SIOCIFGCLONERS64 _IOWR('i', 129, struct if_clonereq64)                  
#endif                     

#define SIOCGIFASYNCMAP _IOWR('i', 124, struct ifreq)                         
#define SIOCSIFASYNCMAP _IOW('i', 125, struct ifreq)                          


#ifdef PRIVATE
#define SIOCSETOT     _IOW('s', 128, int)                             
#endif              

#define SIOCGIFMAC      _IOWR('i', 130, struct ifreq)                   
#define SIOCSIFMAC      _IOW('i', 131, struct ifreq)                    
#define SIOCSIFKPI      _IOW('i', 134, struct ifreq)                                           
#define SIOCGIFKPI      _IOWR('i', 135, struct ifreq)                               

#define SIOCGIFWAKEFLAGS _IOWR('i', 136, struct ifreq)                                        

#ifdef PRIVATE
#define SIOCGIFGETRTREFCNT _IOWR('i', 137, struct ifreq)                                 
#define SIOCGIFLINKQUALITYMETRIC _IOWR('i', 138, struct ifreq)              
#define SIOCSIFOPPORTUNISTIC     _IOWR('i', 139, struct ifreq)                                       
#define SIOCGIFOPPORTUNISTIC     _IOWR('i', 140, struct ifreq)                                       
#define SIOCSETROUTERMODE       _IOWR('i', 141, struct ifreq)                                                     
#define SIOCGIFEFLAGS           _IOWR('i', 142, struct ifreq)                                 
#define SIOCSIFDESC     _IOWR('i', 143, struct if_descreq)
#define SIOCGIFDESC     _IOWR('i', 144, struct if_descreq)
#define SIOCSIFLINKPARAMS _IOWR('i', 145, struct if_linkparamsreq)
#define SIOCGIFLINKPARAMS _IOWR('i', 146, struct if_linkparamsreq)
#define SIOCGIFQUEUESTATS _IOWR('i', 147, struct if_qstatsreq)
#define SIOCSIFTHROTTLE _IOWR('i', 148, struct if_throttlereq)
#define SIOCGIFTHROTTLE _IOWR('i', 149, struct if_throttlereq)

#define SIOCGASSOCIDS   _IOWR('s', 150, struct so_aidreq)                   
#define SIOCGCONNIDS    _IOWR('s', 151, struct so_cidreq)                  
#define SIOCGCONNINFO   _IOWR('s', 152, struct so_cinforeq)                   
#ifdef BSD_KERNEL_PRIVATE
#define SIOCGASSOCIDS32 _IOWR('s', 150, struct so_aidreq32)
#define SIOCGASSOCIDS64 _IOWR('s', 150, struct so_aidreq64)
#define SIOCGCONNIDS32  _IOWR('s', 151, struct so_cidreq32)
#define SIOCGCONNIDS64  _IOWR('s', 151, struct so_cidreq64)
#define SIOCGCONNINFO32 _IOWR('s', 152, struct so_cinforeq32)
#define SIOCGCONNINFO64 _IOWR('s', 152, struct so_cinforeq64)
#endif                         
#define SIOCSCONNORDER  _IOWR('s', 153, struct so_cordreq)                     
#define SIOCGCONNORDER  _IOWR('s', 154, struct so_cordreq)                     

#define SIOCSIFLOG      _IOWR('i', 155, struct ifreq)
#define SIOCGIFLOG      _IOWR('i', 156, struct ifreq)
#define SIOCGIFDELEGATE _IOWR('i', 157, struct ifreq)
#define SIOCGIFLLADDR   _IOWR('i', 158, struct ifreq)                          
#define SIOCGIFTYPE     _IOWR('i', 159, struct ifreq)                         
#define SIOCGIFEXPENSIVE _IOWR('i', 160, struct ifreq)                                   
#define SIOCSIFEXPENSIVE _IOWR('i', 161, struct ifreq)                               
#define SIOCGIF2KCL     _IOWR('i', 162, struct ifreq)                                        
#define SIOCSIF2KCL     _IOWR('i', 163, struct ifreq)
#define SIOCGSTARTDELAY _IOWR('i', 164, struct ifreq)

#define SIOCAIFAGENTID  _IOWR('i', 165, struct if_agentidreq)                      
#define SIOCDIFAGENTID  _IOWR('i', 166, struct if_agentidreq)                         
#define SIOCGIFAGENTIDS _IOWR('i', 167, struct if_agentidsreq)                       
#define SIOCGIFAGENTDATA        _IOWR('i', 168, struct netagent_req)                        

#ifdef BSD_KERNEL_PRIVATE
#define SIOCGIFAGENTIDS32       _IOWR('i', 167, struct if_agentidsreq32)
#define SIOCGIFAGENTIDS64       _IOWR('i', 167, struct if_agentidsreq64)
#define SIOCGIFAGENTDATA32              _IOWR('i', 168, struct netagent_req32)
#define SIOCGIFAGENTDATA64              _IOWR('i', 168, struct netagent_req64)
#endif                         

#define SIOCSIFINTERFACESTATE   _IOWR('i', 169, struct ifreq)                          
#define SIOCGIFINTERFACESTATE   _IOWR('i', 170, struct ifreq)                          
#define SIOCSIFPROBECONNECTIVITY _IOWR('i', 171, struct ifreq)                                              
#define SIOCGIFPROBECONNECTIVITY        _IOWR('i', 172, struct ifreq)                                                 

#endif              
#define SIOCGIFFUNCTIONALTYPE   _IOWR('i', 173, struct ifreq)                                    
#ifdef PRIVATE
#define SIOCSIFNETSIGNATURE     _IOWR('i', 174, struct if_nsreq)
#define SIOCGIFNETSIGNATURE     _IOWR('i', 175, struct if_nsreq)

#define SIOCGECNMODE            _IOWR('i', 176, struct ifreq)
#define SIOCSECNMODE            _IOW('i', 177, struct ifreq)

#define SIOCSIFORDER    _IOWR('i', 178, struct if_order)

#define SIOCSQOSMARKINGMODE     _IOWR('i', 180, struct ifreq)
#define SIOCSFASTLANECAPABLE    SIOCSQOSMARKINGMODE
#define SIOCSQOSMARKINGENABLED  _IOWR('i', 181, struct ifreq)
#define SIOCSFASTLEENABLED      SIOCSQOSMARKINGENABLED
#define SIOCGQOSMARKINGMODE     _IOWR('i', 182, struct ifreq)
#define SIOCGQOSMARKINGENABLED  _IOWR('i', 183, struct ifreq)


#define SIOCSIFTIMESTAMPENABLE  _IOWR('i', 184, struct ifreq)
#define SIOCSIFTIMESTAMPDISABLE _IOWR('i', 185, struct ifreq)
#define SIOCGIFTIMESTAMPENABLED _IOWR('i', 186, struct ifreq)

#define SIOCSIFDISABLEOUTPUT    _IOWR('i', 187, struct ifreq)

#define SIOCSIFSUBFAMILY        _IOWR('i', 188, struct ifreq)

#define SIOCGIFAGENTLIST        _IOWR('i', 190, struct netagentlist_req)                        

#ifdef BSD_KERNEL_PRIVATE
#define SIOCGIFAGENTLIST32              _IOWR('i', 190, struct netagentlist_req32)
#define SIOCGIFAGENTLIST64              _IOWR('i', 190, struct netagentlist_req64)
#endif                         

#define SIOCSIFLOWINTERNET      _IOWR('i', 191, struct ifreq)
#define SIOCGIFLOWINTERNET      _IOWR('i', 192, struct ifreq)

#define SIOCGIFNAT64PREFIX      _IOWR('i', 193, struct if_nat64req)
#define SIOCSIFNAT64PREFIX      _IOWR('i', 194, struct if_nat64req)
#define SIOCGIFNEXUS            _IOWR('i', 195, struct if_nexusreq)
#define SIOCGIFPROTOLIST        _IOWR('i', 196, struct if_protolistreq)                                     
#ifdef BSD_KERNEL_PRIVATE
#define SIOCGIFPROTOLIST32      _IOWR('i', 196, struct if_protolistreq32)
#define SIOCGIFPROTOLIST64      _IOWR('i', 196, struct if_protolistreq64)
#endif                         
#endif              

#define SIOCSIF6LOWPAN  _IOW('i', 196, struct ifreq)                            
#define SIOCGIF6LOWPAN  _IOWR('i', 197, struct ifreq)                           

#ifdef PRIVATE
#define SIOCGIFTCPKAOMAX        _IOWR('i', 198, struct ifreq)                                         
#define SIOCGIFLOWPOWER _IOWR('i', 199, struct ifreq)                       
#define SIOCSIFLOWPOWER _IOWR('i', 200, struct ifreq)                       

#define SIOCGIFCLAT46ADDR       _IOWR('i', 201, struct if_clat46req)

#define SIOCGIFMPKLOG _IOWR('i', 202, struct ifreq)                                     
#define SIOCSIFMPKLOG _IOWR('i', 203, struct ifreq)                                     

#define SIOCGIFCONSTRAINED _IOWR('i', 204, struct ifreq)                                     
#define SIOCSIFCONSTRAINED _IOWR('i', 205, struct ifreq)                                 

#define SIOCGIFXFLAGS           _IOWR('i', 206, struct ifreq)                                 

#define SIOCGIFNOACKPRIO _IOWR('i', 207, struct ifreq)                                               
#define SIOCSIFNOACKPRIO _IOWR('i', 208, struct ifreq)                                                 
#define SIOCGETROUTERMODE _IOWR('i', 209, struct ifreq)                                   

#define SIOCSIFNETWORKID _IOWR('i', 210, struct if_netidreq)                                                     
#endif              

#endif                      
