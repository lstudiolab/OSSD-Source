/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
  
   

#ifndef _I386_TSS_H_
#define _I386_TSS_H_

#include <stdint.h>

  
                          
   
struct i386_tss {
	uint32_t        back_link;                                         
                                                 
	uint32_t        esp0;                                          
	uint32_t        ss0;                                        
	uint32_t        esp1;                                          
	uint32_t        ss1;                                        
	uint32_t        esp2;                                          
	uint32_t        ss2;                                        
	uint32_t        cr3;                                         
                                                              
	uint32_t        eip;
	uint32_t        eflags;
	uint32_t        eax;
	uint32_t        ecx;
	uint32_t        edx;
	uint32_t        ebx;
	uint32_t        esp;                                       
	uint32_t        ebp;
	uint32_t        esi;
	uint32_t        edi;
	uint32_t        es;
	uint32_t        cs;
	uint32_t        ss;                                        
	uint32_t        ds;
	uint32_t        fs;
	uint32_t        gs;
	uint32_t        ldt;                                                
	uint16_t        trace_trap;                                      
	uint16_t        io_bit_map_offset;
	                                   
               
};

  
                                                                     
                                                                    
                                                                               
                                                                        
                                                               
   

struct sysenter_stack {
	uint64_t        sysestack[64];                                         
	uint64_t        top;                                               
};

#pragma pack(4)
struct x86_64_tss {
	uint32_t        reserved1;
	uint64_t        rsp0;                                       
	uint64_t        rsp1;                                       
	uint64_t        rsp2;                                       
	uint32_t        reserved2;
	uint32_t        reserved3;
	uint64_t        ist1;                                        
	uint64_t        ist2;                                        
	uint64_t        ist3;                                        
	uint64_t        ist4;                                        
	uint64_t        ist5;                                        
	uint64_t        ist6;                                        
	uint64_t        ist7;                                        
	uint32_t        reserved4;
	uint32_t        reserved5;
	uint16_t        reserved6;
	uint16_t        io_bit_map_offset;
	                                     
};
#pragma pack()
#endif                    
