/*
 * Copyright (c) 1999-2014 Apple Computer, Inc. All rights reserved.
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

  
                                             
                                
   

#ifndef _MACH_H_
#define _MACH_H_

#define __MACH30__
#define MACH_IPC_FLAVOR UNTYPED

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/mach_interface.h>
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/thread_switch.h>

#include <mach/rpc.h>                                       
#include <mach/mig.h>

#include <mach/mig_errors.h>
#include <mach/mach_error.h>

#include <sys/cdefs.h>

__BEGIN_DECLS
  
                      
   
extern void                     panic_init(mach_port_t);
extern void                     panic(const char *, ...);

extern void                     safe_gets(char *,
    char *,
    int);

extern void                     slot_name(cpu_type_t,
    cpu_subtype_t,
    char **,
    char **);

extern void                     mig_reply_setup(mach_msg_header_t *,
    mach_msg_header_t *);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern void                     mach_msg_destroy(mach_msg_header_t *);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern mach_msg_return_t        mach_msg_receive(mach_msg_header_t *);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern mach_msg_return_t        mach_msg_send(mach_msg_header_t *);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern mach_msg_return_t        mach_msg_server_once(boolean_t (*)
    (mach_msg_header_t *,
    mach_msg_header_t *),
    mach_msg_size_t,
    mach_port_t,
    mach_msg_options_t);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern mach_msg_return_t        mach_msg_server(boolean_t (*)
    (mach_msg_header_t *,
    mach_msg_header_t *),
    mach_msg_size_t,
    mach_port_t,
    mach_msg_options_t);

__WATCHOS_PROHIBITED __TVOS_PROHIBITED
extern mach_msg_return_t        mach_msg_server_importance(boolean_t (*)
    (mach_msg_header_t *,
    mach_msg_header_t *),
    mach_msg_size_t,
    mach_port_t,
    mach_msg_options_t);

  
                               
   
extern kern_return_t    clock_get_res(mach_port_t,
    clock_res_t *);
extern kern_return_t    clock_set_res(mach_port_t,
    clock_res_t);

extern kern_return_t    clock_sleep(mach_port_t,
    int,
    mach_timespec_t,
    mach_timespec_t *);

   
                                     
   

#define VOUCHER_MACH_MSG_API_VERSION 20140205

   
                                    
  
            
                                                                         
   
typedef struct voucher_mach_msg_state_s *voucher_mach_msg_state_t;

   
                                          
  
              
                                                
   
#define VOUCHER_MACH_MSG_STATE_UNCHANGED ((voucher_mach_msg_state_t)~0ul)

   
                                 
  
            
                                                                         
                         
                                                                             
  
              
                                                                        
                                                                          
  
             
                         
  
          
                              
   
extern boolean_t voucher_mach_msg_set(mach_msg_header_t *msg);

   
                                   
  
            
                                                                             
                                                          
                                                                            
           
  
              
                                                                      
                                                                       
                                                                   
  
             
                         
   
extern void voucher_mach_msg_clear(mach_msg_header_t *msg);

   
                                   
  
            
                                                                             
                                                
  
              
                                                                             
                                                            
  
             
                                   
  
          
                                                                              
                         
   
extern voucher_mach_msg_state_t voucher_mach_msg_adopt(mach_msg_header_t *msg);

   
                                    
  
            
                                                                                
  
              
                                                
                                                                                
  
               
                                       
   

extern void voucher_mach_msg_revert(voucher_mach_msg_state_t state);

__END_DECLS

#endif                
