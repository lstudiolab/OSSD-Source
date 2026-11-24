/*
 * Copyright (c) 2000-2003 Apple Computer, Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                      
  
                                           
   

#ifndef _MACH_NOTIFY_H_
#define _MACH_NOTIFY_H_

#include <mach/port.h>
#include <mach/message.h>
#include <mach/ndr.h>

  
                                                              
                                     
   

#define MACH_NOTIFY_FIRST               0100
#define MACH_NOTIFY_PORT_DELETED        (MACH_NOTIFY_FIRST + 001)
                                            
#define MACH_NOTIFY_SEND_POSSIBLE       (MACH_NOTIFY_FIRST + 002)
                                                
#define MACH_NOTIFY_PORT_DESTROYED      (MACH_NOTIFY_FIRST + 005)
                                                       
#define MACH_NOTIFY_NO_SENDERS          (MACH_NOTIFY_FIRST + 006)
                                             
#define MACH_NOTIFY_SEND_ONCE           (MACH_NOTIFY_FIRST + 007)
                                    
#define MACH_NOTIFY_DEAD_NAME           (MACH_NOTIFY_FIRST + 010)
                                                       
#define MACH_NOTIFY_LAST                (MACH_NOTIFY_FIRST + 015)

typedef mach_port_t notify_port_t;

  
                                                                     
                                                                  
                                                                    
                                                                     
                                                              
                                      
   
typedef struct {
	mach_msg_header_t   not_header;
	NDR_record_t        NDR;
	mach_port_name_t not_port;                             
	mach_msg_format_0_trailer_t trailer;
} mach_port_deleted_notification_t;

typedef struct {
	mach_msg_header_t   not_header;
	NDR_record_t        NDR;
	mach_port_name_t not_port;                             
	mach_msg_format_0_trailer_t trailer;
} mach_send_possible_notification_t;

typedef struct {
	mach_msg_header_t   not_header;
	mach_msg_body_t     not_body;
	mach_msg_port_descriptor_t not_port;                                
	mach_msg_format_0_trailer_t trailer;
} mach_port_destroyed_notification_t;

typedef struct {
	mach_msg_header_t   not_header;
	NDR_record_t        NDR;
	mach_msg_type_number_t not_count;
	mach_msg_format_0_trailer_t trailer;
} mach_no_senders_notification_t;

typedef struct {
	mach_msg_header_t   not_header;
	mach_msg_format_0_trailer_t trailer;
} mach_send_once_notification_t;

typedef struct {
	mach_msg_header_t   not_header;
	NDR_record_t        NDR;
	mach_port_name_t not_port;                             
	mach_msg_format_0_trailer_t trailer;
} mach_dead_name_notification_t;

#endif                       
