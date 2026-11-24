/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * ppp_defs.h - PPP definitions.
 *
 * Copyright (c) 1994 The Australian National University.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied. The Australian National University
 * makes no representations about the suitability of this software for
 * any purpose.
 *
 * IN NO EVENT SHALL THE AUSTRALIAN NATIONAL UNIVERSITY BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
 * THE AUSTRALIAN NATIONAL UNIVERSITY HAVE BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * THE AUSTRALIAN NATIONAL UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE AUSTRALIAN NATIONAL UNIVERSITY HAS NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
 * OR MODIFICATIONS.
 *
 */

#ifndef _PPP_DEFS_H_
#define _PPP_DEFS_H_
#include <sys/appleapiopts.h>

  
                       
   
#define PPP_HDRLEN      4                                           
#define PPP_FCSLEN      2                           
#define PPP_MRU         1500                                                

#define PPP_ADDRESS(p)  (((u_char *)(p))[0])
#define PPP_CONTROL(p)  (((u_char *)(p))[1])
#define PPP_PROTOCOL(p) ((((u_char *)(p))[2] << 8) + ((u_char *)(p))[3])

  
                            
   
#define PPP_ALLSTATIONS 0xff                                        
#define PPP_UI          0x03                                
#define PPP_FLAG        0x7e                       
#define PPP_ESCAPE      0x7d                                     
#define PPP_TRANS       0x20                                            

  
                         
   
#define PPP_IP          0x21                           
#define PPP_XNS         0x25                  
#define PPP_AT          0x29                            
#define PPP_IPX         0x2b                                
#define PPP_VJC_COMP    0x2d                           
#define PPP_VJC_UNCOMP  0x2f                             
#define PPP_COMP        0xfd                           
#define PPP_IPCP        0x8021                           
#define PPP_ATCP        0x8029                                  
#define PPP_IPXCP       0x802b                                      
#define PPP_CCP         0x80fd                                    
#define PPP_LCP         0xc021                             
#define PPP_PAP         0xc023                                        
#define PPP_LQR         0xc025                                    
#define PPP_CHAP        0xc223                                              
#define PPP_CBCP        0xc029                                 
#define PPP_IPV6        0x57                                    
#define PPP_IPV6CP      0x8057                             

  
                               
   
#define PPP_INITFCS     0xffff                         
#define PPP_GOODFCS     0xf0b8                            
#define PPP_FCS(fcs, c) (((fcs) >> 8) ^ fcstab[((fcs) ^ (c)) & 0xff])

  
                                                          
   
typedef u_int32_t       ext_accm[8];

  
                                                 
   
enum NPmode {
	NPMODE_PASS,                                         
	NPMODE_DROP,                                          
	NPMODE_ERROR,                                
	NPMODE_QUEUE                                       
};

  
              
   
struct pppstat  {
	unsigned int ppp_ibytes;                     
	unsigned int ppp_ipackets;                       
	unsigned int ppp_ierrors;                     
	unsigned int ppp_obytes;                 
	unsigned int ppp_opackets;                   
	unsigned int ppp_oerrors;                      
};

struct vjstat {
	unsigned int vjs_packets;                       
	unsigned int vjs_compressed;                                  
	unsigned int vjs_searches;                                    
	unsigned int vjs_misses;                                      
	unsigned int vjs_uncompressedin;                                   
	unsigned int vjs_compressedin;                                 
	unsigned int vjs_errorin;                                   
	unsigned int vjs_tossed;                                              
};

struct ppp_stats {
	struct pppstat p;                                 
	struct vjstat vj;                                             
};

struct compstat {
	unsigned int unc_bytes;                               
	unsigned int unc_packets;                                 
	unsigned int comp_bytes;                       
	unsigned int comp_packets;                         
	unsigned int inc_bytes;                           
	unsigned int inc_packets;                             
	unsigned int ratio;                                        
};

struct ppp_comp_stats {
	struct compstat c;                                         
	struct compstat d;                                           
};

  
                                                            
                                           
   
struct ppp_idle {
	time_t xmit_idle;                                           
	time_t recv_idle;                                               
};

#endif                   
