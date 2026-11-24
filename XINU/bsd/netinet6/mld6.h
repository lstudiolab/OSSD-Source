/*
 * Copyright (c) 2010 Apple Computer, Inc. All rights reserved.
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
/*-
 * Copyright (c) 2009 Bruce Simpson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _NETINET6_MLD6_H_
#define _NETINET6_MLD6_H_

  
                                                  
   

                                                 
#define MLD_MINLEN      sizeof(struct icmp6_hdr)

  
                       
                                           
                                        
   
struct mldv2_query {
	struct icmp6_hdr        mld_icmp6_hdr;                     
	struct in6_addr         mld_addr;                                  
	uint8_t         mld_misc;                                           
	uint8_t         mld_qqi;                                            
	uint16_t        mld_numsrc;                                         
	                                            
} __attribute__((__packed__));
#define MLD_V2_QUERY_MINLEN             sizeof(struct mldv2_query)
#define MLD_MRC_EXP(x)                  ((ntohs((x)) >> 12) & 0x0007)
#define MLD_MRC_MANT(x)                 (ntohs((x)) & 0x0fff)
#define MLD_QQIC_EXP(x)                 (((x) >> 4) & 0x07)
#define MLD_QQIC_MANT(x)                ((x) & 0x0f)
#define MLD_QRESV(x)                    (((x) >> 4) & 0x0f)
#define MLD_SFLAG(x)                    (((x) >> 3) & 0x01)
#define MLD_QRV(x)                      ((x) & 0x07)

  
                                       
                                  
   
struct mldv2_report {
	struct icmp6_hdr        mld_icmp6_hdr;
	                                    
} __attribute__((__packed__));
                                   
#define mld_numrecs     mld_icmp6_hdr.icmp6_data16[1]

struct mldv2_record {
	uint8_t                 mr_type;                         
	uint8_t                 mr_datalen;                                   
	uint16_t                mr_numsrc;                             
	struct in6_addr         mr_addr;                                    
	                                            
} __attribute__((__packed__));
#define MLD_V2_REPORT_MAXRECS           65535

  
                      
   
#define MLD_DO_NOTHING                  0                                
#define MLD_MODE_IS_INCLUDE             1                    
#define MLD_MODE_IS_EXCLUDE             2                    
#define MLD_CHANGE_TO_INCLUDE_MODE      3                  
#define MLD_CHANGE_TO_EXCLUDE_MODE      4                  
#define MLD_ALLOW_NEW_SOURCES           5                      
#define MLD_BLOCK_OLD_SOURCES           6                      

  
                     
   
#define MLD_V2_GENERAL_QUERY            1
#define MLD_V2_GROUP_QUERY              2
#define MLD_V2_GROUP_SOURCE_QUERY       3

  
                                                             
   
#define MLD_V1_MAX_RI                   10

  
                                                            
                        
   
#define MLD_TIMER_SCALE                 1000

#endif                        
