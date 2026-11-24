/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
  
   

#ifndef _MACH_I386_EXCEPTION_H_
#define _MACH_I386_EXCEPTION_H_

  
                                           
   

#define EXC_TYPES_COUNT 14                                     

  
                                           
   

#define EXCEPTION_CODE_MAX      2                                       

  
                      
   

#define EXC_I386_INVOP                  1

  
                 
   

#define EXC_I386_DIV                    1
#define EXC_I386_INTO                   2
#define EXC_I386_NOEXT                  3
#define EXC_I386_EXTOVR                 4
#define EXC_I386_EXTERR                 5
#define EXC_I386_EMERR                  6
#define EXC_I386_BOUND                  7
#define EXC_I386_SSEEXTERR              8

  
               
                                               
   

  
                 
   

  
                 
   

#define EXC_I386_SGL                    1
#define EXC_I386_BPT                    2

#define EXC_I386_DIVERR         0                                
#define EXC_I386_SGLSTP         1                          
#define EXC_I386_NMIFLT         2                   
#define EXC_I386_BPTFLT         3                              
#define EXC_I386_INTOFLT        4                                 
#define EXC_I386_BOUNDFLT       5                                    
#define EXC_I386_INVOPFLT       6                                  
#define EXC_I386_NOEXTFLT       7                                         
#define EXC_I386_DBLFLT         8                           
#define EXC_I386_EXTOVRFLT      9                                    
#define EXC_I386_INVTSSFLT      10                              
#define EXC_I386_SEGNPFLT       11                                     
#define EXC_I386_STKFLT         12                         
#define EXC_I386_GPFLT          13                                    
#define EXC_I386_PGFLT          14                        
#define EXC_I386_EXTERRFLT      16                                 
#define EXC_I386_ALIGNFLT       17                           
#define EXC_I386_ENDPERR        33                                        
#define EXC_I386_ENOEXTFLT      32                                    


  
                                    
   
#define EXC_MASK_MACHINE        0

#endif                               
