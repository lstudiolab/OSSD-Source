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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 *  Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *  Additional fixes for AVR contributed by:
 *        Colin O'Flynn coflynn@newae.com
 *        Eric Gnoske egnoske@gmail.com
 *        Blake Leverett bleverett@gmail.com
 *        Mike Vidales mavida404@gmail.com
 *        Kevin Brown kbrown3@uccs.edu
 *        Nate Bohlmann nate@elfwerks.com
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
 */

   
                     
        
   

   
                                                               
        
   
   
         
                                                        
  
                                                                   
          
  
   

              
#ifndef FRAME_802154_H
#define FRAME_802154_H

#include "contiki-conf.h"

#include <stdint.h>
#include <sys/types.h>
#ifdef IEEE802154_CONF_PANID
#define IEEE802154_PANID        IEEE802154_CONF_PANID
#else                            
#define IEEE802154_PANID        0xABCD
#endif                            

                      

                                                                                                    
                                        
      
   
#define FRAME802154_BEACONFRAME         (0x00)
#define FRAME802154_DATAFRAME           (0x01)
#define FRAME802154_ACKFRAME            (0x02)
#define FRAME802154_CMDFRAME            (0x03)

#define FRAME802154_BEACONREQ           (0x07)

#define FRAME802154_IEEERESERVED        (0x00)
#define FRAME802154_NOADDR              (0x00)                                                  
#define FRAME802154_SHORTADDRMODE       (0x02)
#define FRAME802154_LONGADDRMODE        (0x03)

#define FRAME802154_NOBEACONS           (0x0F)

#define FRAME802154_BROADCASTADDR       (0xFFFF)
#define FRAME802154_BROADCASTPANDID     (0xFFFF)

#define FRAME802154_IEEE802154_2003     (0x00)
#define FRAME802154_IEEE802154_2006     (0x01)

#define FRAME802154_SECURITY_LEVEL_NONE         (0)
#define FRAME802154_SECURITY_LEVEL_MIC_32       (1)
#define FRAME802154_SECURITY_LEVEL_MIC_64       (2)
#define FRAME802154_SECURITY_LEVEL_MIC_128      (3)
#define FRAME802154_SECURITY_LEVEL_ENC          (4)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_32   (5)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_64   (6)
#define FRAME802154_SECURITY_LEVEL_ENC_MIC_128  (7)

#define FRAME802154_IMPLICIT_KEY                (0)
#define FRAME802154_1_BYTE_KEY_ID_MODE          (1)
#define FRAME802154_5_BYTE_KEY_ID_MODE          (2)
#define FRAME802154_9_BYTE_KEY_ID_MODE          (3)

   
                                                                                
                                                                       
                                  
  
                            
                                                             
                                                             
                                                                
                                                                
                                                             
   

   
                                                                 
   
typedef struct {
	uint8_t frame_type;                                                     
	uint8_t security_enabled;                                                       
	uint8_t frame_pending;                                                        
	uint8_t ack_required;                                              
	uint8_t panid_compression;                                            
	                                                     
	uint8_t dest_addr_mode;                                                         
	uint8_t frame_version;                                          
	uint8_t src_addr_mode;                                                     
} frame802154_fcf_t;

                                                                                                  
typedef struct {
	uint8_t  security_level;                                   
	uint8_t  key_id_mode;                                      
	uint8_t  reserved;                                         
} frame802154_scf_t;

typedef union {
	uint32_t u32;
	uint16_t u16[2];
	uint8_t u8[4];
} frame802154_frame_counter_t;

typedef union {
	uint16_t u16[4];
	uint8_t u8[8];
} frame802154_key_source_t;

                                          
typedef struct {
	frame802154_scf_t security_control;                                         
	frame802154_frame_counter_t frame_counter;                                         
	frame802154_key_source_t key_source;                                  
	uint8_t key_index;                                                   
} frame802154_aux_hdr_t;

                                                                       
                                                                       
                              
   
struct frame802154 {
	                                                                                      
                                                                                          
                                                                                             
	uint8_t dest_addr[8];                                      
	uint8_t src_addr[8];                                  
	frame802154_fcf_t fcf;                                      
	uint8_t seq;                                           
	uint16_t dest_pid;                                        
	uint16_t src_pid;                                    
	frame802154_aux_hdr_t aux_hdr;                             
	                                                                     
	uint32_t payload_len;                                               
};
typedef struct frame802154 frame802154_t;

                

int frame802154_hdrlen(frame802154_t *p);
int frame802154_create(frame802154_t *p, uint8_t *buf);
size_t frame802154_parse(uint8_t *data, size_t length, frame802154_t *pf, uint8_t **payload);

         
#endif                     
         
         
