/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
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

#ifndef _SYS_ULOCK_H
#define _SYS_ULOCK_H

#include <mach/mach_port.h>
#include <sys/cdefs.h>
#include <stdint.h>

__BEGIN_DECLS

#if PRIVATE

#ifdef XNU_KERNEL_PRIVATE
extern mach_port_name_t ipc_entry_name_mask(mach_port_name_t name);

static __inline mach_port_name_t
ulock_owner_value_to_port_name(uint32_t uval)
{
	  
                                                                   
                                                                     
                                                                     
                                       
    
	return ipc_entry_name_mask((mach_port_name_t)uval);
}
#else
static __inline mach_port_name_t
ulock_owner_value_to_port_name(uint32_t uval)
{
	return uval | 0x3;
}
#endif

#ifndef KERNEL

extern int __ulock_wait(uint32_t operation, void *addr, uint64_t value,
    uint32_t timeout);                                                       
extern int __ulock_wait2(uint32_t operation, void *addr, uint64_t value,
    uint64_t timeout, uint64_t value2);
extern int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value);

#endif              

  
                                                    
  
                                                    
                                        
   
#define UL_COMPARE_AND_WAIT             1
#define UL_UNFAIR_LOCK                  2
#define UL_COMPARE_AND_WAIT_SHARED      3
#define UL_UNFAIR_LOCK64_SHARED         4
#define UL_COMPARE_AND_WAIT64           5
#define UL_COMPARE_AND_WAIT64_SHARED    6
                    
#define UL_OSSPINLOCK                   UL_COMPARE_AND_WAIT
#define UL_HANDOFFLOCK                  UL_UNFAIR_LOCK
                                                                                 
#define UL_DEBUG_SIMULATE_COPYIN_FAULT  253
#define UL_DEBUG_HASH_DUMP_ALL          254
#define UL_DEBUG_HASH_DUMP_PID          255

  
                                                            
   
#define ULF_WAKE_ALL                    0x00000100
#define ULF_WAKE_THREAD                 0x00000200
#define ULF_WAKE_ALLOW_NON_OWNER        0x00000400

  
                                                             
  
                                        
                                                                                
                                                                              
                        
  
                               
                                   
  
                                
                                                                             
              
   
#define ULF_WAIT_WORKQ_DATA_CONTENTION  0x00010000
#define ULF_WAIT_CANCEL_POINT           0x00020000
#define ULF_WAIT_ADAPTIVE_SPIN          0x00040000

  
                                                    
   
#define ULF_NO_ERRNO                    0x01000000

  
        
   
#define UL_OPCODE_MASK          0x000000FF
#define UL_FLAGS_MASK           0xFFFFFF00
#define ULF_GENERIC_MASK        0xFFFF0000

#define ULF_WAIT_MASK           (ULF_NO_ERRNO | \
	                         ULF_WAIT_WORKQ_DATA_CONTENTION | \
	                         ULF_WAIT_CANCEL_POINT | ULF_WAIT_ADAPTIVE_SPIN)

#define ULF_WAKE_MASK           (ULF_NO_ERRNO | \
	                         ULF_WAKE_ALL | \
	                         ULF_WAKE_THREAD | \
	                         ULF_WAKE_ALLOW_NON_OWNER)

#endif              

__END_DECLS

#endif
