/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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

#include <architecture/i386/pio.h>
#include <i386/panic_notify.h>
#include <kern/assert.h>
#include <pexpert/pexpert.h>
#include <stdint.h>

  
                                                             
                                         
   
static uint16_t panic_io_port = 0;

  
                                                                      
                                                                      
                
                                                                     
                                          
                                                                            
   
static uint16_t pvpanic_io_port = 0;

void
panic_notify_init(void)
{
	(void) PE_parse_boot_argn("panic_io_port", &panic_io_port, sizeof(panic_io_port));

	  
       
                                                                      
                                                             
                                                                       
                                   
    
	(void) PE_parse_boot_argn("pvpanic_io_port", &pvpanic_io_port, sizeof(pvpanic_io_port));
}

void
panic_notify(void)
{
	if (panic_io_port != 0) {
		(void) inb(panic_io_port);
	}

	if (pvpanic_io_port != 0 &&
	    (inb(pvpanic_io_port) & PVPANIC_NOTIFICATION_BIT) != 0) {
		outb(pvpanic_io_port, PVPANIC_NOTIFICATION_BIT);
	}
}
