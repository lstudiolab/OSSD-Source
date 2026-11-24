/*
 * Copyright (c) 2000-2012 Apple Computer, Inc. All rights reserved.
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
  
   

  
                                                              
   

#ifndef _KERN_AST_H_
#define _KERN_AST_H_


#include <kern/assert.h>
#include <kern/macro_help.h>
#include <kern/spl.h>

  
                                                                
                                                                  
                                                                       
  
                                                        
                               
   
typedef uint32_t ast_t;

  
                                                             
                                                             
                                                                 
          
  
                                                               
                                                              
                                                               
  
                                                             
                                                               
                          
  
                                                                
                                                                
                                                        
  
                                                               
                                                           
                                                         
                                                               
                                                              
   

  
                        
                                                                        
                                                                                      
   
#define AST_PREEMPT             0x01
#define AST_QUANTUM             0x02
#define AST_URGENT              0x04
#define AST_HANDOFF             0x08
#define AST_YIELD               0x10
#define AST_APC                 0x20                            
#define AST_LEDGER              0x40
#define AST_BSD                 0x80
#define AST_KPERF               0x100                         
#define AST_MACF                0x200                              
#define AST_RESET_PCS           0x400                           
#define AST_ARCADE              0x800                                   
#define AST_GUARD               0x1000
#define AST_TELEMETRY_USER      0x2000                                                              
#define AST_TELEMETRY_KERNEL    0x4000                                                           
#define AST_TELEMETRY_PMI       0x8000                                         
#define AST_SFI                 0x10000                                                                
#define AST_DTRACE              0x20000
#define AST_TELEMETRY_IO        0x40000                                         
#define AST_KEVENT              0x80000
#define AST_REBALANCE           0x100000                                                 
#define AST_UNQUIESCE           0x200000                                                               

#define AST_NONE                0x00
#define AST_ALL                 (~AST_NONE)

#define AST_SCHEDULING  (AST_PREEMPTION | AST_YIELD | AST_HANDOFF)
#define AST_PREEMPTION  (AST_PREEMPT | AST_QUANTUM | AST_URGENT)

#define AST_TELEMETRY_ALL (AST_TELEMETRY_USER | AST_TELEMETRY_KERNEL | \
	        AST_TELEMETRY_PMI | AST_TELEMETRY_IO)

                                                               
#define AST_PER_THREAD  (AST_APC | AST_BSD | AST_MACF | AST_RESET_PCS | \
	AST_ARCADE | AST_LEDGER | AST_GUARD | AST_TELEMETRY_ALL | AST_KEVENT)

                                                    
extern void ast_taken_kernel(void);

                                                                                                    
extern void ast_taken_user(void);

                            
extern void ast_check(processor_t processor);

                                                
extern ast_t *ast_pending(void);

                                        
extern void ast_on(ast_t reasons);

                                          
extern void ast_off(ast_t reasons);

                                                        
extern ast_t ast_consume(ast_t reasons);

                                                     
extern ast_t ast_peek(ast_t reasons);

                                                                            
extern void ast_context(thread_t thread);

                                                             
extern void ast_propagate(thread_t thread);

  
                                              
  
                                                                           
                                                                 
  
                                    
   
#define thread_ast_set(act, reason)     ((void)os_atomic_or(&(act)->ast, (reason), relaxed))
#define thread_ast_clear(act, reason)   ((void)os_atomic_andnot(&(act)->ast, (reason), relaxed))

#ifdef MACH_BSD

extern void act_set_astbsd(thread_t);
extern void bsd_ast(thread_t);

#endif               

#ifdef CONFIG_DTRACE
extern void ast_dtrace_on(void);
extern void dtrace_ast(void);
#endif                    

extern void kevent_ast(thread_t thread, uint16_t bits);
extern void act_set_astkevent(thread_t thread, uint16_t bits);
extern uint16_t act_clear_astkevent(thread_t thread, uint16_t bits);
extern void act_set_ast_reset_pcs(thread_t thread);

#endif                    
