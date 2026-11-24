/*
 * Copyright (c) 2017-2021 Apple Inc. All rights reserved.
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
 *
 *  Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *  Additional fixes for AVR contributed by:
 *
 *      Colin O'Flynn coflynn@newae.com
 *      Eric Gnoske egnoske@gmail.com
 *      Blake Leverett bleverett@gmail.com
 *      Mike Vidales mavida404@gmail.com
 *      Kevin Brown kbrown3@uccs.edu
 *      Nate Bohlmann nate@elfwerks.com
 *
 *  Additional fixes for MSP430 contributed by:
 *        Joakim Eriksson
 *        Niclas Finne
 *        Nicolas Tsiftes
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
  
                                                                     
                             
   

   
         
                                                        
  
                                                                   
          
   

   
                            
       
   

#include "cc.h"
#include "frame802154.h"
                                    
#include "linkaddr.h"
#include <string.h>

   
                                                                                            
                                                                                
   
typedef struct {
	uint8_t dest_pid_len;                                                          
	uint8_t dest_addr_len;                                                          
	uint8_t src_pid_len;                                                      
	uint8_t src_addr_len;                                                      
	uint8_t aux_sec_len;                                                            
} field_length_t;

                                                                                
CC_INLINE static uint8_t
addr_len(uint8_t mode)
{
	switch (mode) {
	case FRAME802154_SHORTADDRMODE:                              
		return 2;
	case FRAME802154_LONGADDRMODE:                               
		return 8;
	default:
		return 0;
	}
}
                                                                                
#if LLSEC802154_USES_EXPLICIT_KEYS
static uint8_t
get_key_id_len(uint8_t key_id_mode)
{
	switch (key_id_mode) {
	case FRAME802154_1_BYTE_KEY_ID_MODE:
		return 1;
	case FRAME802154_5_BYTE_KEY_ID_MODE:
		return 5;
	case FRAME802154_9_BYTE_KEY_ID_MODE:
		return 9;
	default:
		return 0;
	}
}
#endif                                     
                                                                                
static void
field_len(frame802154_t *p, field_length_t *flen)
{
	                        
	memset(flen, 0, sizeof(field_length_t));

	                                                                 
	if (p->fcf.dest_addr_mode & 3) {
		flen->dest_pid_len = 2;
	}
	if (p->fcf.src_addr_mode & 3) {
		flen->src_pid_len = 2;
	}

	                                                                   
	if (p->fcf.dest_addr_mode & 3 && p->fcf.src_addr_mode & 3 &&
	    p->src_pid == p->dest_pid) {
		p->fcf.panid_compression = 1;

		                                         
		flen->src_pid_len = 0;
	} else {
		p->fcf.panid_compression = 0;
	}

	                               
	flen->dest_addr_len = addr_len(p->fcf.dest_addr_mode & 3);
	flen->src_addr_len = addr_len(p->fcf.src_addr_mode & 3);

#if LLSEC802154_SECURITY_LEVEL
	                         
	if (p->fcf.security_enabled & 1) {
		flen->aux_sec_len = 5
#if LLSEC802154_USES_EXPLICIT_KEYS
		    + get_key_id_len(p->aux_hdr.security_control.key_id_mode);
#endif                                     
		;
	}
#endif                                 
}
                                                                                
   
                                                                        
                                                                             
  
                                                                    
                   
  
                                            
   
int
frame802154_hdrlen(frame802154_t *p)
{
	field_length_t flen;
	field_len(p, &flen);
	return 3 + flen.dest_pid_len + flen.dest_addr_len +
	       flen.src_pid_len + flen.src_addr_len + flen.aux_sec_len;
}
                                                                                
   
                                                                            
                                                                             
  
                                                                  
                   
  
                                                           
  
                                           
   
int
frame802154_create(frame802154_t *p, uint8_t *buf)
{
	int c;
	field_length_t flen;
	uint8_t pos;
#if LLSEC802154_USES_EXPLICIT_KEYS
	uint8_t key_id_mode;
#endif                                     

	field_len(p, &flen);

	                                                                
	                                             
	buf[0] = (uint8_t)((p->fcf.frame_type & 7) |
	    ((p->fcf.security_enabled & 1) << 3) |
	    ((p->fcf.frame_pending & 1) << 4) |
	    ((p->fcf.ack_required & 1) << 5) |
	    ((p->fcf.panid_compression & 1) << 6));
	buf[1] = (uint8_t)(((p->fcf.dest_addr_mode & 3) << 2) |
	    ((p->fcf.frame_version & 3) << 4) |
	    ((p->fcf.src_addr_mode & 3) << 6));

	                     
	buf[2] = p->seq;
	pos = 3;

	                        
	if (flen.dest_pid_len == 2) {
		buf[pos++] = p->dest_pid & 0xff;
		buf[pos++] = (p->dest_pid >> 8) & 0xff;
	}

	                         
	for (c = flen.dest_addr_len; c > 0; c--) {
		buf[pos++] = p->dest_addr[c - 1];
	}

	                   
	if (flen.src_pid_len == 2) {
		buf[pos++] = p->src_pid & 0xff;
		buf[pos++] = (p->src_pid >> 8) & 0xff;
	}

	                    
	for (c = flen.src_addr_len; c > 0; c--) {
		buf[pos++] = p->src_addr[c - 1];
	}

#if LLSEC802154_SECURITY_LEVEL
	                
	if (flen.aux_sec_len) {
		buf[pos++] = p->aux_hdr.security_control.security_level
#if LLSEC802154_USES_EXPLICIT_KEYS
		    | (p->aux_hdr.security_control.key_id_mode << 3)
#endif                                     
		;
		memcpy(buf + pos, p->aux_hdr.frame_counter.u8, 4);
		pos += 4;

#if LLSEC802154_USES_EXPLICIT_KEYS
		key_id_mode = p->aux_hdr.security_control.key_id_mode;
		if (key_id_mode) {
			c = (key_id_mode - 1) * 4;
			memcpy(buf + pos, p->aux_hdr.key_source.u8, c);
			pos += c;
			buf[pos++] = p->aux_hdr.key_index;
		}
#endif                                     
	}
#endif                                 

	return (int)pos;
}
                                                                                
   
                                                                      
                                                             
                             
  
                                                    
                                          
                                                                              
   
size_t
frame802154_parse(uint8_t *data, size_t len, frame802154_t *pf, uint8_t **payload)
{
	uint8_t *p;
	frame802154_fcf_t fcf;
	size_t c;
#if LLSEC802154_USES_EXPLICIT_KEYS
	uint8_t key_id_mode;
#endif                                     

	if (len < 3) {
		*payload = NULL;
		return 0;
	}

	p = data;

	                    
	fcf.frame_type = p[0] & 7;
	fcf.security_enabled = (p[0] >> 3) & 1;
	fcf.frame_pending = (p[0] >> 4) & 1;
	fcf.ack_required = (p[0] >> 5) & 1;
	fcf.panid_compression = (p[0] >> 6) & 1;

	fcf.dest_addr_mode = (p[1] >> 2) & 3;
	fcf.frame_version = (p[1] >> 4) & 3;
	fcf.src_addr_mode = (p[1] >> 6) & 3;

	                         
	memcpy(&pf->fcf, &fcf, sizeof(frame802154_fcf_t));
	pf->seq = p[2];
	p += 3;                                                         

	                                 
	if (fcf.dest_addr_mode) {
		                     
		pf->dest_pid = (uint16_t)(p[0] + (p[1] << 8));
		p += 2;

		                         
		                                           
		                                  
		                                               
		           
		                 
		if (fcf.dest_addr_mode == FRAME802154_SHORTADDRMODE) {
			linkaddr_copy((linkaddr_t *)(uintptr_t)&(pf->dest_addr), &linkaddr_null);
			pf->dest_addr[0] = p[1];
			pf->dest_addr[1] = p[0];
			p += 2;
		} else if (fcf.dest_addr_mode == FRAME802154_LONGADDRMODE) {
			for (c = 0; c < 8; c++) {
				pf->dest_addr[c] = p[7 - c];
			}
			p += 8;
		}
	} else {
		linkaddr_copy((linkaddr_t *)(uintptr_t)&(pf->dest_addr), &linkaddr_null);
		pf->dest_pid = 0;
	}

	                            
	if (fcf.src_addr_mode) {
		                
		if (!fcf.panid_compression) {
			pf->src_pid = (uint16_t)(p[0] + (p[1] << 8));
			p += 2;
		} else {
			pf->src_pid = pf->dest_pid;
		}

		                    
		                                          
		                                  
		                                              
		           
		                 
		if (fcf.src_addr_mode == FRAME802154_SHORTADDRMODE) {
			linkaddr_copy((linkaddr_t *)(uintptr_t)&(pf->src_addr), &linkaddr_null);
			pf->src_addr[0] = p[1];
			pf->src_addr[1] = p[0];
			p += 2;
		} else if (fcf.src_addr_mode == FRAME802154_LONGADDRMODE) {
			for (c = 0; c < 8; c++) {
				pf->src_addr[c] = p[7 - c];
			}
			p += 8;
		}
	} else {
		linkaddr_copy((linkaddr_t *)(uintptr_t)&(pf->src_addr), &linkaddr_null);
		pf->src_pid = 0;
	}

#if LLSEC802154_SECURITY_LEVEL
	if (fcf.security_enabled) {
		pf->aux_hdr.security_control.security_level = p[0] & 7;
#if LLSEC802154_USES_EXPLICIT_KEYS
		pf->aux_hdr.security_control.key_id_mode = (p[0] >> 3) & 3;
#endif                                     
		p += 1;

		memcpy(pf->aux_hdr.frame_counter.u8, p, 4);
		p += 4;

#if LLSEC802154_USES_EXPLICIT_KEYS
		key_id_mode = pf->aux_hdr.security_control.key_id_mode;
		if (key_id_mode) {
			c = (key_id_mode - 1) * 4;
			memcpy(pf->aux_hdr.key_source.u8, p, c);
			p += c;
			pf->aux_hdr.key_index = p[0];
			p += 1;
		}
#endif                                     
	}
#endif                                 

	                   
	c = p - data;


	if (c <= len) {
		                    
		pf->payload_len = (int)(len - c);
		             
		*payload = p;
	} else {
		*payload = NULL;
	}

	                                        
	return c > len ? 0 : c;
}
           
