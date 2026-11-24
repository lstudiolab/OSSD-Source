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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	architecture/i386/reg_help.h
 *	Author:	Mike DeMoney, NeXT Computer, Inc.
 *      Modified for i386 by: Bruce Martin, NeXT Computer, Inc.
 *
 *	This header file defines cpp macros useful for defining
 *	machine register and doing machine-level operations.
 *
 * HISTORY
 * 10-Mar-92  Bruce Martin (bmartin@next.com)
 *	Adapted to i386
 * 23-Jan-91  Mike DeMoney (mike@next.com)
 *	Created.
 */

#ifndef	_ARCH_I386_REG_HELP_H_
#define	_ARCH_I386_REG_HELP_H_

                             
#define	BITS_WIDTH(msb, lsb)	((msb)-(lsb)+1)
#define	BIT_WIDTH(pos)		(1)	                                   

                   
#define	MKMASK(width, offset)	(((unsigned)-1)>>(32-(width))<<(offset))
#define	BITSMASK(msb, lsb)	MKMASK(BITS_WIDTH(msb, lsb), lsb & 0x1f)
#define	BITMASK(pos)		MKMASK(BIT_WIDTH(pos), pos & 0x1f)

                        
#if	__ASSEMBLER__
# define	REG_ADDR(type, addr)	(addr)
#else	                   
# define	REG_ADDR(type, addr)	(*(volatile type *)(addr))
#endif	                   

                                       
#define	CONTENTS(foo)	(*(unsigned *) &(foo))

                                                  
#define	STACK_INCR	4
#define	ROUND_FRAME(x)	((((unsigned)(x)) + STACK_INCR - 1) & ~(STACK_INCR-1))

                                                                     
#define	__STR(x)	#x		                         
#define	STRINGIFY(x)	__STR(x)

  
                                         
                                                               
                 
  
         
               
                 
                   
                
      
     
  
                                                               
                 
   
#define	REG_PAIR_DEF(type0, name0, type1, name1)		\
	struct {						\
		type0	name0 __attribute__(( aligned(8) ));	\
		type1	name1;					\
	} name0##_##name1

  
                                                                       
                     
  
         
                                
                                 
                                  
      
                                            
   
#define	REG_PAIR(name0, name1, the_name)			\
	name0##_##name1.the_name

#endif	                            
