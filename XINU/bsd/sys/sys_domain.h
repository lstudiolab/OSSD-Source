/*
 * Copyright (c) 2000-2005, 2012, 2014 Apple Inc. All rights reserved.
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


#ifndef _SYSTEM_DOMAIN_H_
#define _SYSTEM_DOMAIN_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#ifdef KERNEL_PRIVATE
#include <sys/sysctl.h>
#endif                     

                            
#define SYSPROTO_EVENT          1                                   

                             
#define SYSPROTO_CONTROL        2                                    
#define AF_SYS_CONTROL          2                                           

                                  
struct sockaddr_sys {
	u_char          ss_len;                                          
	u_char          ss_family;                     
	u_int16_t       ss_sysaddr;                                        
	u_int32_t       ss_reserved[7];                                   
};

#ifdef PRIVATE
struct  xsystmgen {
	u_int32_t       xg_len;                               
	u_int64_t       xg_count;                                        
	u_int64_t       xg_gen;                                    
	u_int64_t       xg_sogen;                                            
};
#endif              

#ifdef KERNEL_PRIVATE

extern struct domain *systemdomain;

SYSCTL_DECL(_net_systm);

                                                    
__BEGIN_DECLS
void kern_event_init(struct domain *);
void kern_control_init(struct domain *);
__END_DECLS
#endif                     

#endif                        
