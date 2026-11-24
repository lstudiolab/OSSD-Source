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
 * ppp_comp.h - Definitions for doing PPP packet compression.
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

#ifndef _NET_PPP_COMP_H
#define _NET_PPP_COMP_H
#ifdef KERNEL_PRIVATE
  
                                                            
                               
   
#ifndef DO_BSD_COMPRESS
#define DO_BSD_COMPRESS 1                                             
#endif
#ifndef DO_DEFLATE
#define DO_DEFLATE      1                                        
#endif
#define DO_PREDICTOR_1  0
#define DO_PREDICTOR_2  0

  
                                                          
   
#if PACKETPTR
struct compressor {
	int     compress_proto;                                      

	                                                     
	void    *(*comp_alloc)(u_char *options, int opt_len);
	                                     
	void    (*comp_free)(void *state);
	                             
	int     (*comp_init)(void *state, u_char *options, int opt_len,
	    int unit, int hdrlen, int debug);
	                        
	void    (*comp_reset)(void *state);
	                       
	int     (*compress)(void *state, PACKETPTR *mret,
	    PACKETPTR mp, int orig_len, int max_len);
	                                   
	void    (*comp_stat)(void *state, struct compstat *stats);

	                                                      
	void    *(*decomp_alloc)(u_char *options, int opt_len);
	                                       
	void    (*decomp_free)(void *state);
	                               
	int     (*decomp_init)(void *state, u_char *options, int opt_len,
	    int unit, int hdrlen, int mru, int debug);
	                          
	void    (*decomp_reset)(void *state);
	                          
	int     (*decompress)(void *state, PACKETPTR mp, PACKETPTR *dmpp);
	                                                        
	void    (*incomp)(void *state, PACKETPTR mp);
	                                     
	void    (*decomp_stat)(void *state, struct compstat *stats);
};
#endif                

  
                                        
                                                                    
                                                                       
                                                                         
                             
                                         
   
#define DECOMP_OK               0                               
#define DECOMP_ERROR            1                                          
#define DECOMP_FATALERROR       2                                         

  
             
   
#define CCP_CONFREQ     1
#define CCP_CONFACK     2
#define CCP_TERMREQ     5
#define CCP_TERMACK     6
#define CCP_RESETREQ    14
#define CCP_RESETACK    15

  
                               
   
#define CCP_MAX_OPTION_LENGTH   32

  
                         
   
#define CCP_CODE(dp)            ((dp)[0])
#define CCP_ID(dp)              ((dp)[1])
#define CCP_LENGTH(dp)          (((dp)[2] << 8) + (dp)[3])
#define CCP_HDRLEN              4

#define CCP_OPT_CODE(dp)        ((dp)[0])
#define CCP_OPT_LENGTH(dp)      ((dp)[1])
#define CCP_OPT_MINLEN          2

  
                                
   
#define CI_BSD_COMPRESS         21                                           
#define CILEN_BSD_COMPRESS      3                                     

                                                                         
#define BSD_NBITS(x)            ((x) & 0x1F)                                  
#define BSD_VERSION(x)          ((x) >> 5)                                    
#define BSD_CURRENT_VERSION     1                                           
#define BSD_MAKE_OPT(v, n)      (((v) << 5) | (n))

#define BSD_MIN_BITS            9                                         
#define BSD_MAX_BITS            15                                       

  
                           
   
#define CI_DEFLATE              26                                     
#define CI_DEFLATE_DRAFT        24                                            
#define CILEN_DEFLATE           4                                        

#define DEFLATE_MIN_SIZE        8
#define DEFLATE_MAX_SIZE        15
#define DEFLATE_METHOD_VAL      8
#define DEFLATE_SIZE(x)         (((x) >> 4) + DEFLATE_MIN_SIZE)
#define DEFLATE_METHOD(x)       ((x) & 0x0F)
#define DEFLATE_MAKE_OPT(w)     ((((w) - DEFLATE_MIN_SIZE) << 4) \
	                         + DEFLATE_METHOD_VAL)
#define DEFLATE_CHK_SEQUENCE    0

  
                                                                  
   
#define CI_PREDICTOR_1          1                                          
#define CILEN_PREDICTOR_1       2                                        
#define CI_PREDICTOR_2          2                                          
#define CILEN_PREDICTOR_2       2                                        

#endif                     
#endif                      
