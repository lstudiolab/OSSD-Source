/*
 * Copyright (c) 2003-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

  
                      
                                                  
                  
  
                                    
   

#include <TargetConditionals.h>
#include <mach/error.h>
#include "errorlib.h"
#if !TARGET_OS_DRIVERKIT
#include "err_libkern.sub"
#include "err_iokit.sub"
#endif                        
#include "err_ipc.sub"
#include "err_kern.sub"
#include "err_mach_ipc.sub"
#include "err_server.sub"
#include "err_us.sub"

const struct error_system _mach_errors[err_max_system + 1] = {
	                 
	{
		.max_sub = errlib_count(err_os_sub),
		.bad_sub = "(operating system/?) unknown subsystem error",
		.subsystem = err_os_sub,
	},
	               
	{
		.max_sub = errlib_count(err_us_sub),
		.bad_sub = "(user space/?) unknown subsystem error",
		.subsystem = err_us_sub,
	},
	                   
	{
		.max_sub = errlib_count(err_server_sub),
		.bad_sub = "(server/?) unknown subsystem error",
		.subsystem = err_server_sub,
	},
	                       
	{
		.max_sub = errlib_count(err_ipc_sub),
		.bad_sub = "(ipc/?) unknown subsystem error",
		.subsystem = err_ipc_sub,
	},
	                     
	{
		.max_sub = errlib_count(err_mach_ipc_sub),
		.bad_sub = "(ipc/?) unknown subsystem error",
		.subsystem = err_mach_ipc_sub,
	},

	           errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,

	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,

	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,

	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,

#if !TARGET_OS_DRIVERKIT
	                       
	{
		.max_sub = errlib_count(err_libkern_sub),
		.bad_sub = "(libkern/?) unknown subsystem error",
		.subsystem = err_libkern_sub,
	},

	                     
	{
		.max_sub = errlib_count(err_iokit_sub),
		.bad_sub = "(iokit/?) unknown subsystem error",
		.subsystem = err_iokit_sub,
		.map_table = err_iokit_sub_map,
		.map_count = errlib_count(err_iokit_sub_map)
	},
#else
	           errorlib_system_null,            errorlib_system_null,
#endif                       

	           errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
	           errorlib_system_null,            errorlib_system_null,
};

                                                       
