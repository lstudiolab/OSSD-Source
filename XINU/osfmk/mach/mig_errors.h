/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
  
   
  
                                  
  
   

#ifndef _MACH_MIG_ERRORS_H_
#define _MACH_MIG_ERRORS_H_

#include <mach/mig.h>
#include <mach/ndr.h>
#include <mach/message.h>
#include <mach/kern_return.h>

#include <sys/cdefs.h>

  
                                                                 
                                                          
                                                             
                                                                  
                                                             
                                                        
   

#define MIG_TYPE_ERROR          -300                                   
#define MIG_REPLY_MISMATCH      -301                                
#define MIG_REMOTE_ERROR        -302                               
#define MIG_BAD_ID              -303                                
#define MIG_BAD_ARGUMENTS       -304                                   
#define MIG_NO_REPLY            -305                                 
#define MIG_EXCEPTION           -306                                 
#define MIG_ARRAY_TOO_LARGE     -307                                
#define MIG_SERVER_DIED         -308                     
#define MIG_TRAILER_ERROR       -309                                       

  
                                                         
                                                         
                                                          
   
#pragma pack(4)
typedef struct {
	mach_msg_header_t       Head;
	NDR_record_t            NDR;
	kern_return_t           RetCode;
} mig_reply_error_t;
#pragma pack()


__BEGIN_DECLS

#if !defined(__NDR_convert__mig_reply_error_t__defined)
#define __NDR_convert__mig_reply_error_t__defined

static __inline__ void
__NDR_convert__mig_reply_error_t(__unused mig_reply_error_t *x)
{
#if defined(__NDR_convert__int_rep__kern_return_t__defined)
	if (x->NDR.int_rep != NDR_record.int_rep) {
		__NDR_convert__int_rep__kern_return_t(&x->RetCode, x->NDR.int_rep);
	}
#endif                                                     
}
#endif                                                          

__END_DECLS

#endif                           
